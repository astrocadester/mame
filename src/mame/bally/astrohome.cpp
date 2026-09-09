// license:BSD-3-Clause
// copyright-holders:Nicola Salmoria, Mike Coates, Frank Palazzolo, Aaron Giles, Dirk Best
/****************************************************************************

    Bally Astrocade consumer hardware

****************************************************************************/

#include "emu.h"
#include "astrocde.h"

#include "cpu/z80/z80.h"
#include "machine/ram.h"
#include "sound/astrocde.h"

#include "bus/astrocde/slot.h"
#include "bus/astrocde/rom.h"
#include "bus/astrocde/exp.h"
#include "bus/astrocde/ram.h"
#include "bus/astrocde/ctrl.h"
#include "bus/astrocde/accessory.h"

#include "softlist_dev.h"
#include "speaker.h"


namespace {

class astrocde_home_state : public astrocde_state
{
public:
	astrocde_home_state(const machine_config &mconfig, device_type type, const char *tag)
		: astrocde_state(mconfig, type, tag)
		, m_cart(*this, "cartslot")
		, m_exp(*this, "exp")
		, m_ctrl(*this, "ctrl%u", 1U)
		, m_accessory(*this, "accessory")
		, m_keypad(*this, "KEYPAD%u", 0U)
	{ }

	void astrocde(machine_config &config);

	void init_astrocde();

protected:
	virtual void machine_start() override ATTR_COLD;

private:
	uint8_t inputs_r(offs_t offset);
	void astrohome_palette(palette_device &palette) const;

	void astrocade_io(address_map &map) ATTR_COLD;
	void astrocade_mem(address_map &map) ATTR_COLD;

	required_device<astrocade_cart_slot_device> m_cart;
	required_device<astrocade_exp_device> m_exp;
	required_device_array<astrocade_ctrl_port_device, 4> m_ctrl;
	required_device<astrocade_accessory_port_device> m_accessory;
	required_ioport_array<4> m_keypad;
};


/*********************************************************************************
 *
 *  Memory maps
 *
 * $0000 to $1FFF:  8K on-board ROM (could be one of three available BIOS dumps)
 * $2000 to $3FFF:  8K cartridge ROM
 * $4000 to $4FFF:  4K screen RAM
 * $5000 to $FFFF:  44K address space not available in standard machine.  With a
 * sufficiently large RAM expansion, all of this RAM can be added, and accessed
 * by an extended BASIC program.  Bally and Astrocade BASIC can access from
 * $5000 to $7FFF if available.
 *
 *********************************************************************************/

void astrocde_home_state::astrocade_mem(address_map &map)
{
	map(0x0000, 0x0fff).rom().w(FUNC(astrocde_home_state::astrocade_funcgen_w));
	map(0x1000, 0x3fff).rom(); /* Star Fortress writes in here?? */
	map(0x4000, 0x4fff).ram().share("videoram"); /* ASG */
	//map(0x5000, 0xffff).rw("exp", FUNC(astrocade_exp_device::read), FUNC(astrocade_exp_device::write));
}


void astrocde_home_state::astrocade_io(address_map &map)
{
	map(0x00, 0x0f).select(0xff00).rw(FUNC(astrocde_state::video_register_r), FUNC(astrocde_state::video_register_w));
	map(0x10, 0x1f).select(0xff00).r(m_astrocade_sound[0], FUNC(astrocade_io_device::read));
	map(0x10, 0x18).select(0xff00).w(m_astrocade_sound[0], FUNC(astrocade_io_device::write));
	map(0x19, 0x19).mirror(0xff00).w(FUNC(astrocde_state::expand_register_w));
}


/*************************************
 *
 *  Home console palette
 *
 *************************************/

void astrocde_home_state::astrohome_palette(palette_device &palette) const
{
	/*
	    The consumer hardware exposes 256 colors as 32 hues with eight
	    luminance levels.  This initial RGB calibration separates the home
	    console from the arcade boards and keeps every color directly
	    adjustable as improved hardware measurements become available.

	    AstroBASIC uses $A7 for green shift, $5F for red shift, $0F for
	    blue shift, and $77 for words/gold shift.
	*/
	static constexpr rgb_t colors[32][8] =
	{
		// HUE $00
		{
			rgb_t(0x00, 0x00, 0x00), rgb_t(0x24, 0x24, 0x24),
			rgb_t(0x49, 0x49, 0x49), rgb_t(0x6d, 0x6d, 0x6d),
			rgb_t(0x92, 0x92, 0x92), rgb_t(0xb6, 0xb6, 0xb6),
			rgb_t(0xdb, 0xdb, 0xdb), rgb_t(0xff, 0xff, 0xff)
		},

		// HUE $01
		{
			rgb_t(0x0d, 0x1b, 0x5b), rgb_t(0x10, 0x20, 0x6d),
			rgb_t(0x13, 0x25, 0x7f), rgb_t(0x15, 0x2b, 0x90),
			rgb_t(0x18, 0x30, 0xa2), rgb_t(0x1b, 0x35, 0xb4),
			rgb_t(0x1d, 0x3b, 0xc6), rgb_t(0x20, 0x40, 0xd8)
		},

		// HUE $02
		{
			rgb_t(0x11, 0x0d, 0x5b), rgb_t(0x14, 0x10, 0x6d),
			rgb_t(0x17, 0x13, 0x7f), rgb_t(0x1b, 0x15, 0x90),
			rgb_t(0x1e, 0x18, 0xa2), rgb_t(0x21, 0x1b, 0xb4),
			rgb_t(0x25, 0x1d, 0xc6), rgb_t(0x28, 0x20, 0xd8)
		},

		// HUE $03
		{
			rgb_t(0x22, 0x0d, 0x5b), rgb_t(0x28, 0x10, 0x6d),
			rgb_t(0x2f, 0x13, 0x7f), rgb_t(0x35, 0x15, 0x90),
			rgb_t(0x3c, 0x18, 0xa2), rgb_t(0x43, 0x1b, 0xb4),
			rgb_t(0x49, 0x1d, 0xc6), rgb_t(0x50, 0x20, 0xd8)
		},

		// HUE $04
		{
			rgb_t(0x32, 0x0d, 0x5b), rgb_t(0x3c, 0x10, 0x6d),
			rgb_t(0x46, 0x13, 0x7f), rgb_t(0x50, 0x15, 0x90),
			rgb_t(0x5a, 0x18, 0xa2), rgb_t(0x64, 0x1b, 0xb4),
			rgb_t(0x6e, 0x1d, 0xc6), rgb_t(0x78, 0x20, 0xd8)
		},

		// HUE $05
		{
			rgb_t(0x43, 0x0d, 0x5b), rgb_t(0x50, 0x10, 0x6d),
			rgb_t(0x5e, 0x13, 0x7f), rgb_t(0x6b, 0x15, 0x90),
			rgb_t(0x78, 0x18, 0xa2), rgb_t(0x85, 0x1b, 0xb4),
			rgb_t(0x93, 0x1d, 0xc6), rgb_t(0xa0, 0x20, 0xd8)
		},

		// HUE $06
		{
			rgb_t(0x54, 0x0d, 0x5b), rgb_t(0x65, 0x10, 0x6d),
			rgb_t(0x75, 0x13, 0x7f), rgb_t(0x86, 0x15, 0x90),
			rgb_t(0x96, 0x18, 0xa2), rgb_t(0xa7, 0x1b, 0xb4),
			rgb_t(0xb7, 0x1d, 0xc6), rgb_t(0xc8, 0x20, 0xd8)
		},

		// HUE $07
		{
			rgb_t(0x5b, 0x0d, 0x51), rgb_t(0x6d, 0x10, 0x61),
			rgb_t(0x7f, 0x13, 0x70), rgb_t(0x90, 0x15, 0x80),
			rgb_t(0xa2, 0x18, 0x90), rgb_t(0xb4, 0x1b, 0xa0),
			rgb_t(0xc6, 0x1d, 0xb0), rgb_t(0xd8, 0x20, 0xc0)
		},

		// HUE $08
		{
			rgb_t(0x5b, 0x0d, 0x40), rgb_t(0x6d, 0x10, 0x4c),
			rgb_t(0x7f, 0x13, 0x59), rgb_t(0x90, 0x15, 0x66),
			rgb_t(0xa2, 0x18, 0x72), rgb_t(0xb4, 0x1b, 0x7f),
			rgb_t(0xc6, 0x1d, 0x8b), rgb_t(0xd8, 0x20, 0x98)
		},

		// HUE $09
		{
			rgb_t(0x5b, 0x0d, 0x2f), rgb_t(0x6d, 0x10, 0x38),
			rgb_t(0x7f, 0x13, 0x42), rgb_t(0x90, 0x15, 0x4b),
			rgb_t(0xa2, 0x18, 0x54), rgb_t(0xb4, 0x1b, 0x5d),
			rgb_t(0xc6, 0x1d, 0x67), rgb_t(0xd8, 0x20, 0x70)
		},

		// HUE $0A
		{
			rgb_t(0x5b, 0x0d, 0x1e), rgb_t(0x6d, 0x10, 0x24),
			rgb_t(0x7f, 0x13, 0x2a), rgb_t(0x90, 0x15, 0x30),
			rgb_t(0xa2, 0x18, 0x36), rgb_t(0xb4, 0x1b, 0x3c),
			rgb_t(0xc6, 0x1d, 0x42), rgb_t(0xd8, 0x20, 0x48)
		},

		// HUE $0B
		{
			rgb_t(0x5b, 0x0d, 0x0d), rgb_t(0x6d, 0x10, 0x10),
			rgb_t(0x7f, 0x13, 0x13), rgb_t(0x90, 0x15, 0x15),
			rgb_t(0xa2, 0x18, 0x18), rgb_t(0xb4, 0x1b, 0x1b),
			rgb_t(0xc6, 0x1d, 0x1d), rgb_t(0xd8, 0x20, 0x20)
		},

		// HUE $0C
		{
			rgb_t(0x5b, 0x20, 0x0d), rgb_t(0x6d, 0x27, 0x10),
			rgb_t(0x7f, 0x2d, 0x13), rgb_t(0x90, 0x33, 0x15),
			rgb_t(0xa2, 0x3a, 0x18), rgb_t(0xb4, 0x40, 0x1b),
			rgb_t(0xc6, 0x47, 0x1d), rgb_t(0xd8, 0x4d, 0x20)
		},

		// HUE $0D
		{
			rgb_t(0x5b, 0x34, 0x0d), rgb_t(0x6d, 0x3e, 0x10),
			rgb_t(0x7f, 0x48, 0x13), rgb_t(0x90, 0x52, 0x15),
			rgb_t(0xa2, 0x5c, 0x18), rgb_t(0xb4, 0x67, 0x1b),
			rgb_t(0xc6, 0x71, 0x1d), rgb_t(0xd8, 0x7b, 0x20)
		},

		// HUE $0E
		{
			rgb_t(0x5b, 0x47, 0x0d), rgb_t(0x6d, 0x54, 0x10),
			rgb_t(0x7f, 0x62, 0x13), rgb_t(0x90, 0x70, 0x15),
			rgb_t(0xa2, 0x7e, 0x18), rgb_t(0xb4, 0x8c, 0x1b),
			rgb_t(0xc6, 0x9a, 0x1d), rgb_t(0xd8, 0xa8, 0x20)
		},

		// HUE $0F
		{
			rgb_t(0x5b, 0x59, 0x0d), rgb_t(0x6d, 0x6b, 0x10),
			rgb_t(0x7f, 0x7c, 0x13), rgb_t(0x90, 0x8e, 0x15),
			rgb_t(0xa2, 0x9f, 0x18), rgb_t(0xb4, 0xb1, 0x1b),
			rgb_t(0xc6, 0xc2, 0x1d), rgb_t(0xd8, 0xd4, 0x20)
		},

		// HUE $10
		{
			rgb_t(0x4a, 0x5b, 0x0d), rgb_t(0x59, 0x6d, 0x10),
			rgb_t(0x67, 0x7f, 0x13), rgb_t(0x76, 0x90, 0x15),
			rgb_t(0x84, 0xa2, 0x18), rgb_t(0x93, 0xb4, 0x1b),
			rgb_t(0xa1, 0xc6, 0x1d), rgb_t(0xb0, 0xd8, 0x20)
		},

		// HUE $11
		{
			rgb_t(0x37, 0x5b, 0x0d), rgb_t(0x42, 0x6d, 0x10),
			rgb_t(0x4d, 0x7f, 0x13), rgb_t(0x58, 0x90, 0x15),
			rgb_t(0x63, 0xa2, 0x18), rgb_t(0x6e, 0xb4, 0x1b),
			rgb_t(0x79, 0xc6, 0x1d), rgb_t(0x84, 0xd8, 0x20)
		},

		// HUE $12
		{
			rgb_t(0x25, 0x5b, 0x0d), rgb_t(0x2c, 0x6d, 0x10),
			rgb_t(0x34, 0x7f, 0x13), rgb_t(0x3b, 0x90, 0x15),
			rgb_t(0x42, 0xa2, 0x18), rgb_t(0x49, 0xb4, 0x1b),
			rgb_t(0x51, 0xc6, 0x1d), rgb_t(0x58, 0xd8, 0x20)
		},

		// HUE $13
		{
			rgb_t(0x12, 0x5b, 0x0d), rgb_t(0x16, 0x6d, 0x10),
			rgb_t(0x1a, 0x7f, 0x13), rgb_t(0x1d, 0x90, 0x15),
			rgb_t(0x21, 0xa2, 0x18), rgb_t(0x25, 0xb4, 0x1b),
			rgb_t(0x28, 0xc6, 0x1d), rgb_t(0x2c, 0xd8, 0x20)
		},

		// HUE $14
		{
			rgb_t(0x0d, 0x5b, 0x1b), rgb_t(0x10, 0x6d, 0x20),
			rgb_t(0x13, 0x7f, 0x25), rgb_t(0x15, 0x90, 0x2b),
			rgb_t(0x18, 0xa2, 0x30), rgb_t(0x1b, 0xb4, 0x35),
			rgb_t(0x1d, 0xc6, 0x3b), rgb_t(0x20, 0xd8, 0x40)
		},

		// HUE $15
		{
			rgb_t(0x0d, 0x5b, 0x25), rgb_t(0x10, 0x6d, 0x2c),
			rgb_t(0x13, 0x7f, 0x33), rgb_t(0x15, 0x90, 0x3a),
			rgb_t(0x18, 0xa2, 0x41), rgb_t(0x1b, 0xb4, 0x49),
			rgb_t(0x1d, 0xc6, 0x50), rgb_t(0x20, 0xd8, 0x57)
		},

		// HUE $16
		{
			rgb_t(0x0d, 0x5b, 0x2f), rgb_t(0x10, 0x6d, 0x38),
			rgb_t(0x13, 0x7f, 0x41), rgb_t(0x15, 0x90, 0x4a),
			rgb_t(0x18, 0xa2, 0x53), rgb_t(0x1b, 0xb4, 0x5d),
			rgb_t(0x1d, 0xc6, 0x66), rgb_t(0x20, 0xd8, 0x6f)
		},

		// HUE $17
		{
			rgb_t(0x0d, 0x5b, 0x38), rgb_t(0x10, 0x6d, 0x43),
			rgb_t(0x13, 0x7f, 0x4e), rgb_t(0x15, 0x90, 0x5a),
			rgb_t(0x18, 0xa2, 0x65), rgb_t(0x1b, 0xb4, 0x70),
			rgb_t(0x1d, 0xc6, 0x7b), rgb_t(0x20, 0xd8, 0x86)
		},

		// HUE $18
		{
			rgb_t(0x0d, 0x5b, 0x42), rgb_t(0x10, 0x6d, 0x4f),
			rgb_t(0x13, 0x7f, 0x5d), rgb_t(0x15, 0x90, 0x6a),
			rgb_t(0x18, 0xa2, 0x77), rgb_t(0x1b, 0xb4, 0x84),
			rgb_t(0x1d, 0xc6, 0x91), rgb_t(0x20, 0xd8, 0x9e)
		},

		// HUE $19
		{
			rgb_t(0x0d, 0x5b, 0x4c), rgb_t(0x10, 0x6d, 0x5b),
			rgb_t(0x13, 0x7f, 0x6a), rgb_t(0x15, 0x90, 0x79),
			rgb_t(0x18, 0xa2, 0x88), rgb_t(0x1b, 0xb4, 0x97),
			rgb_t(0x1d, 0xc6, 0xa6), rgb_t(0x20, 0xd8, 0xb5)
		},

		// HUE $1A
		{
			rgb_t(0x0d, 0x5b, 0x56), rgb_t(0x10, 0x6d, 0x67),
			rgb_t(0x13, 0x7f, 0x77), rgb_t(0x15, 0x90, 0x88),
			rgb_t(0x18, 0xa2, 0x99), rgb_t(0x1b, 0xb4, 0xaa),
			rgb_t(0x1d, 0xc6, 0xbb), rgb_t(0x20, 0xd8, 0xcc)
		},

		// HUE $1B
		{
			rgb_t(0x0d, 0x56, 0x5b), rgb_t(0x10, 0x67, 0x6d),
			rgb_t(0x13, 0x77, 0x7f), rgb_t(0x15, 0x88, 0x90),
			rgb_t(0x18, 0x99, 0xa2), rgb_t(0x1b, 0xaa, 0xb4),
			rgb_t(0x1d, 0xbb, 0xc6), rgb_t(0x20, 0xcc, 0xd8)
		},

		// HUE $1C
		{
			rgb_t(0x0d, 0x4c, 0x5b), rgb_t(0x10, 0x5b, 0x6d),
			rgb_t(0x13, 0x6a, 0x7f), rgb_t(0x15, 0x79, 0x90),
			rgb_t(0x18, 0x88, 0xa2), rgb_t(0x1b, 0x97, 0xb4),
			rgb_t(0x1d, 0xa6, 0xc6), rgb_t(0x20, 0xb5, 0xd8)
		},

		// HUE $1D
		{
			rgb_t(0x0d, 0x42, 0x5b), rgb_t(0x10, 0x4f, 0x6d),
			rgb_t(0x13, 0x5d, 0x7f), rgb_t(0x15, 0x6a, 0x90),
			rgb_t(0x18, 0x77, 0xa2), rgb_t(0x1b, 0x84, 0xb4),
			rgb_t(0x1d, 0x91, 0xc6), rgb_t(0x20, 0x9e, 0xd8)
		},

		// HUE $1E
		{
			rgb_t(0x0d, 0x38, 0x5b), rgb_t(0x10, 0x43, 0x6d),
			rgb_t(0x13, 0x4e, 0x7f), rgb_t(0x15, 0x5a, 0x90),
			rgb_t(0x18, 0x65, 0xa2), rgb_t(0x1b, 0x70, 0xb4),
			rgb_t(0x1d, 0x7b, 0xc6), rgb_t(0x20, 0x86, 0xd8)
		},

		// HUE $1F
		{
			rgb_t(0x0d, 0x2f, 0x5b), rgb_t(0x10, 0x38, 0x6d),
			rgb_t(0x13, 0x41, 0x7f), rgb_t(0x15, 0x4a, 0x90),
			rgb_t(0x18, 0x53, 0xa2), rgb_t(0x1b, 0x5d, 0xb4),
			rgb_t(0x1d, 0x66, 0xc6), rgb_t(0x20, 0x6f, 0xd8)
		}
	};

	for (unsigned hue = 0; hue < 32; ++hue)
	{
		for (unsigned luma = 0; luma < 8; ++luma)
		{
			unsigned const color = (hue << 3) | luma;
			unsigned const pen = color << 1;

			palette.set_pen_color(pen, colors[hue][luma]);

			// Odd entries provide the arcade sparkle circuit's additional
			// luminance resolution and are not used by the home console.
			palette.set_pen_color(pen | 1, colors[hue][luma]);
		}
	}
}

/*************************************
 *
 *  Input ports
 *
 *
 *  The Astrocade has ports for four hand controllers.  Each controller has a
 *  knob on top that can be simultaneously pushed as an eight-way joystick and
 *  twisted as a paddle, in addition to a trigger button.  The knob can twist
 *  through about 270 degrees, registering 256 unique positions.  It does not
 *  autocenter.  When selecting options on the menu, twisting the knob to the
 *  right gives lower numbers, and twisting to the left gives larger numbers.
 *  Paddle games like Clowns have more intuitive behavior -- twisting to the
 *  right moves the character right.
 *
 *  There is a 24-key keypad on the system itself (6 rows, 4 columns).  It is
 *  labeled for the built-in calculator, but overlays were released for other
 *  programs, the most popular being the BASIC cartridges, which allowed a
 *  large number of inputs by making the bottom row shift buttons.  The labels
 *  below first list the calculator key, then the BASIC keys in the order of no
 *  shift, GREEN shift, RED shift, BLUE shift, WORDS shift.
 *
 *************************************/

uint8_t astrocde_home_state::inputs_r(offs_t offset)
{
	if (BIT(offset, 2))
		return m_keypad[offset & 3]->read();
	else
		return m_ctrl[offset & 3]->read_handle();
}

static INPUT_PORTS_START( astrocde )
	PORT_START("KEYPAD0")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME(u8"%   ÷         [   ]   LIST") PORT_CODE(KEYCODE_O)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("/   x     J   K   L   NEXT") PORT_CODE(KEYCODE_SLASH_PAD)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("x   -     V   W   X   IF") PORT_CODE(KEYCODE_ASTERISK)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("-   +     &   @   *   GOTO") PORT_CODE(KEYCODE_MINUS_PAD)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("+   =     #   %   :   PRINT") PORT_CODE(KEYCODE_PLUS_PAD)
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("=   WORDS Shift") PORT_CODE(KEYCODE_ENTER_PAD)
	PORT_BIT(0xc0, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START("KEYPAD1")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME(u8"\u2193   HALT              RUN") PORT_CODE(KEYCODE_PGDN) // U+2193 = ↓
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("CH  9     G   H   I   STEP") PORT_CODE(KEYCODE_H)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("9   6     S   T   U   RND") PORT_CODE(KEYCODE_9)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME(u8"6   3     \u2191   .   \u2193   BOX") PORT_CODE(KEYCODE_6) // U+2191 = ↑, U+2193 = ↓
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("3   ERASE (   ;   )") PORT_CODE(KEYCODE_3)
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME(".   BLUE Shift") PORT_CODE(KEYCODE_STOP)
	PORT_BIT(0xc0, IP_ACTIVE_HIGH, IPT_UNUSED)

	PORT_START("KEYPAD2")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME(u8"\u2191   PAUSE     /   \\") PORT_CODE(KEYCODE_PGUP) // U+2191 = ↑
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("MS  8     D   E   F   TO") PORT_CODE(KEYCODE_S)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("8   5     P   Q   R   RETN") PORT_CODE(KEYCODE_8)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME(u8"5   2     \u2190   '   \u2192   LINE") PORT_CODE(KEYCODE_5) // U+2190 = ←,  U+2192 = →
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("2   0     <   \"   >   INPUT") PORT_CODE(KEYCODE_2)
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("0   RED Shift") PORT_CODE(KEYCODE_0)
	PORT_BIT(0xc0, IP_ACTIVE_HIGH, IPT_UNUSED)

	PORT_START("KEYPAD3")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("C   GO                +10") PORT_CODE(KEYCODE_C)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("MR  7     A   B   C   FOR") PORT_CODE(KEYCODE_R)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("7   4     M   N   O   GOSB") PORT_CODE(KEYCODE_7)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("4   1     Y   Z   !   CLEAR") PORT_CODE(KEYCODE_4)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("1   SPACE $   ,   ?") PORT_CODE(KEYCODE_1)
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("CE  GREEN Shift") PORT_CODE(KEYCODE_E)
	PORT_BIT(0xc0, IP_ACTIVE_HIGH, IPT_UNUSED)
INPUT_PORTS_END


/*************************************
 *
 *  Machine drivers
 *
 *************************************/

static void astrocade_cart(device_slot_interface &device)
{
	device.option_add_internal("rom",       ASTROCADE_ROM_STD);
	device.option_add_internal("rom_256k",  ASTROCADE_ROM_256K);
	device.option_add_internal("rom_512k",  ASTROCADE_ROM_512K);
	device.option_add_internal("rom_cass",  ASTROCADE_ROM_CASS);
}

static void astrocade_exp(device_slot_interface &device)
{
	device.option_add("blue_ram_4k",   ASTROCADE_BLUERAM_4K);
	device.option_add("blue_ram_16k",  ASTROCADE_BLUERAM_16K);
	device.option_add("blue_ram_32k",  ASTROCADE_BLUERAM_32K);
	device.option_add("viper_sys1",    ASTROCADE_VIPER_SYS1);
	device.option_add("lil_white_ram", ASTROCADE_WHITERAM);
	device.option_add("rl64_ram",      ASTROCADE_RL64RAM);
}


void astrocde_home_state::astrocde(machine_config &config)
{
	/* basic machine hardware */
	Z80(config, m_maincpu, ASTROCADE_CLOCK/4); /* 1.789 MHz */
	m_maincpu->set_addrmap(AS_PROGRAM, &astrocde_home_state::astrocade_mem);
	m_maincpu->set_addrmap(AS_IO, &astrocde_home_state::astrocade_io);

	config.set_perfect_quantum(m_maincpu);

	/* video hardware */
	SCREEN(config, m_screen);
	m_screen->set_raw(ASTROCADE_CLOCK, 455, 0, 352, 262, 0, 240);
	m_screen->set_screen_update(FUNC(astrocde_state::screen_update_astrocde));
	m_screen->set_palette(m_palette);

	PALETTE(config, "palette", FUNC(astrocde_home_state::astrohome_palette), 512);

	/* control ports */
	for (uint32_t port = 0; port < 4; port++)
	{
		ASTROCADE_CTRL_PORT(config, m_ctrl[port], astrocade_controllers, port == 0 ? "joy" : nullptr);
		m_ctrl[port]->ltpen_handler().set(FUNC(astrocde_home_state::lightpen_trigger_w));
	}

	/* sound hardware */
	SPEAKER(config, "mono").front_center();
	ASTROCADE_IO(config, m_astrocade_sound[0], ASTROCADE_CLOCK/4);
	m_astrocade_sound[0]->si_cb().set(FUNC(astrocde_home_state::inputs_r));
	m_astrocade_sound[0]->pot_cb<0>().set(m_ctrl[0], FUNC(astrocade_ctrl_port_device::read_knob));
	m_astrocade_sound[0]->pot_cb<1>().set(m_ctrl[1], FUNC(astrocade_ctrl_port_device::read_knob));
	m_astrocade_sound[0]->pot_cb<2>().set(m_ctrl[2], FUNC(astrocade_ctrl_port_device::read_knob));
	m_astrocade_sound[0]->pot_cb<3>().set(m_ctrl[3], FUNC(astrocade_ctrl_port_device::read_knob));
	m_astrocade_sound[0]->add_route(ALL_OUTPUTS, "mono", 1.0);

	/* expansion port */
	ASTROCADE_EXP_SLOT(config, m_exp, astrocade_exp, nullptr);

	/* cartridge */
	ASTROCADE_CART_SLOT(config, m_cart, astrocade_cart, nullptr);

	/* cartridge */
	ASTROCADE_ACCESSORY_PORT(config, m_accessory, m_screen, astrocade_accessories, nullptr);
	m_accessory->ltpen_handler().set(FUNC(astrocde_home_state::lightpen_trigger_w));

	/* Software lists */
	SOFTWARE_LIST(config, "cart_list").set_original("astrocde");
}


/*************************************
 *
 *  ROM definitions
 *
 *************************************/

ROM_START( astrocde )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "astro.bin",  0x0000, 0x2000, CRC(ebc77f3a) SHA1(b902c941997c9d150a560435bf517c6a28137ecc) )
ROM_END

ROM_START( astrocdl )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "ballyhlc.bin",  0x0000, 0x2000, CRC(d7c517ba) SHA1(6b2bef5d970e54ed204549f58ba6d197a8bfd3cc) )
ROM_END

ROM_START( astrocdw )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "bioswhit.bin",  0x0000, 0x2000, CRC(6eb53e79) SHA1(d84341feec1a0a0e8aa6151b649bc3cf6ef69fbf) )
ROM_END


/*************************************
 *
 *  Driver initialization
 *
 *************************************/

void astrocde_home_state::init_astrocde()
{
	m_video_config = AC_SOUND_PRESENT;
}

void astrocde_home_state::machine_start()
{
	if (m_cart->exists())
		m_maincpu->space(AS_PROGRAM).install_read_handler(0x2000, 0x3fff, read8sm_delegate(*m_cart, FUNC(astrocade_cart_slot_device::read_rom)));

	// if no RAM is mounted and the handlers are installed, the system starts with garbage on screen and a RESET is necessary
	// thus, install RAM only if an expansion is mounted
	if (m_exp->get_card_mounted())
	{
		m_maincpu->space(AS_PROGRAM).install_readwrite_handler(0x5000, 0xffff, read8sm_delegate(*m_exp, FUNC(astrocade_exp_device::read)), write8sm_delegate(*m_exp, FUNC(astrocade_exp_device::write)));
		m_maincpu->space(AS_IO).install_readwrite_handler(0x0080, 0x00ff, 0x0000, 0x0000, 0xff00, read8sm_delegate(*m_exp, FUNC(astrocade_exp_device::read_io)), write8sm_delegate(*m_exp, FUNC(astrocade_exp_device::write_io)));
	}
}

} // Anonymous namespace


/*************************************
 *
 *  Driver definitions
 *
 *************************************/

/*    YEAR  NAME      PARENT    COMPAT  MACHINE   INPUT     CLASS                INIT           COMPANY                FULLNAME                       FLAGS */
CONS( 1978, astrocde, 0,        0,      astrocde, astrocde, astrocde_home_state, init_astrocde, "Bally Manufacturing", "Bally Professional Arcade",   MACHINE_SUPPORTS_SAVE )
CONS( 1977, astrocdl, astrocde, 0,      astrocde, astrocde, astrocde_home_state, init_astrocde, "Bally Manufacturing", "Bally Home Library Computer", MACHINE_SUPPORTS_SAVE )
CONS( 1977, astrocdw, astrocde, 0,      astrocde, astrocde, astrocde_home_state, init_astrocde, "Bally Manufacturing", "Bally Computer System",       MACHINE_SUPPORTS_SAVE )
