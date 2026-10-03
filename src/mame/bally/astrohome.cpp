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
	    The home console color byte contains five hue bits and three
	    luminance bits.  UV1 supplies VIDEO, R-Y and B-Y signals to the
	    television encoder, unlike the arcade hardware's direct RGB output.

	    Ryland's astrocade_measured_palette 2026-09-24 composite
		https://groups.io/g/ballyalley/topic/fix_for_7618_astrocde_and/121154924
	*/	
	static constexpr rgb_t colors[32][8] =
	{
		// HUE $00
		{
			rgb_t(0x00, 0x00, 0x00), rgb_t(0x08, 0x0b, 0x05),
			rgb_t(0x26, 0x29, 0x20), rgb_t(0x65, 0x6a, 0x60),
			rgb_t(0x9b, 0x9f, 0x98), rgb_t(0xc3, 0xc6, 0xc2),
			rgb_t(0xe2, 0xe4, 0xe2), rgb_t(0xff, 0xff, 0xff)
		},
		// HUE $01
		{
			rgb_t(0x00, 0x28, 0xad), rgb_t(0x1e, 0x1c, 0xcc),
			rgb_t(0x3f, 0x25, 0xed), rgb_t(0x70, 0x40, 0xff),
			rgb_t(0x8f, 0x66, 0xff), rgb_t(0x9f, 0x7d, 0xff),
			rgb_t(0xa8, 0x8a, 0xff), rgb_t(0xb2, 0x9a, 0xff)
		},
		// HUE $02
		{
			rgb_t(0x3b, 0x19, 0xac), rgb_t(0x54, 0x1f, 0xc6),
			rgb_t(0x72, 0x28, 0xe5), rgb_t(0x9c, 0x3f, 0xff),
			rgb_t(0xb0, 0x5e, 0xff), rgb_t(0xb9, 0x6f, 0xff),
			rgb_t(0xbd, 0x7a, 0xff), rgb_t(0xc3, 0x8a, 0xff)
		},
		// HUE $03
		{
			rgb_t(0x69, 0x1e, 0x9c), rgb_t(0x83, 0x24, 0xb7),
			rgb_t(0xa1, 0x2d, 0xd7), rgb_t(0xcb, 0x40, 0xff),
			rgb_t(0xd4, 0x55, 0xff), rgb_t(0xd6, 0x62, 0xff),
			rgb_t(0xd6, 0x6c, 0xff), rgb_t(0xd9, 0x7f, 0xff)
		},
		// HUE $04
		{
			rgb_t(0x93, 0x23, 0x88), rgb_t(0xad, 0x2a, 0xa4),
			rgb_t(0xc8, 0x34, 0xc5), rgb_t(0xfe, 0x42, 0xff),
			rgb_t(0xfc, 0x4d, 0xff), rgb_t(0xf6, 0x56, 0xff),
			rgb_t(0xf3, 0x61, 0xff), rgb_t(0xf0, 0x72, 0xff)
		},
		// HUE $05
		{
			rgb_t(0xb4, 0x29, 0x70), rgb_t(0xcb, 0x31, 0x8b),
			rgb_t(0xe7, 0x38, 0xad), rgb_t(0xff, 0x48, 0xd3),
			rgb_t(0xff, 0x46, 0xdd), rgb_t(0xff, 0x44, 0xe7),
			rgb_t(0xff, 0x50, 0xee), rgb_t(0xff, 0x61, 0xf4)
		},
		// HUE $06
		{
			rgb_t(0xcd, 0x2c, 0x5a), rgb_t(0xe3, 0x37, 0x79),
			rgb_t(0xfd, 0x3d, 0x98), rgb_t(0xff, 0x4b, 0xb4),
			rgb_t(0xff, 0x4d, 0xc1), rgb_t(0xff, 0x48, 0xce),
			rgb_t(0xff, 0x4f, 0xd9), rgb_t(0xff, 0x60, 0xe2)
		},
		// HUE $07
		{
			rgb_t(0xce, 0x31, 0x3e), rgb_t(0xf1, 0x39, 0x66),
			rgb_t(0xff, 0x44, 0x80), rgb_t(0xff, 0x50, 0x99),
			rgb_t(0xff, 0x50, 0xaa), rgb_t(0xff, 0x49, 0xb9),
			rgb_t(0xff, 0x53, 0xc5), rgb_t(0xff, 0x61, 0xd0)
		},
		// HUE $08
		{
			rgb_t(0xe3, 0x32, 0x1f), rgb_t(0xf9, 0x3a, 0x3a),
			rgb_t(0xff, 0x43, 0x57), rgb_t(0xff, 0x53, 0x7d),
			rgb_t(0xff, 0x51, 0x93), rgb_t(0xff, 0x4c, 0xa3),
			rgb_t(0xff, 0x59, 0xb1), rgb_t(0xff, 0x69, 0xbd)
		},
		// HUE $09
		{
			rgb_t(0xe3, 0x33, 0x0b), rgb_t(0xfb, 0x38, 0x18),
			rgb_t(0xff, 0x44, 0x41), rgb_t(0xff, 0x53, 0x61),
			rgb_t(0xff, 0x5a, 0x7a), rgb_t(0xff, 0x5a, 0x8b),
			rgb_t(0xff, 0x68, 0x9c), rgb_t(0xff, 0x7b, 0xae)
		},
		// HUE $0A
		{
			rgb_t(0xd4, 0x2c, 0x0a), rgb_t(0xf6, 0x37, 0x11),
			rgb_t(0xff, 0x44, 0x29), rgb_t(0xff, 0x54, 0x49),
			rgb_t(0xff, 0x5f, 0x63), rgb_t(0xff, 0x64, 0x76),
			rgb_t(0xff, 0x78, 0x8d), rgb_t(0xff, 0x8c, 0xa3)
		},
		// HUE $0B
		{
			rgb_t(0xc2, 0x30, 0x05), rgb_t(0xe7, 0x34, 0x0c),
			rgb_t(0xff, 0x3c, 0x16), rgb_t(0xff, 0x53, 0x33),
			rgb_t(0xff, 0x65, 0x4d), rgb_t(0xff, 0x72, 0x62),
			rgb_t(0xff, 0x84, 0x7d), rgb_t(0xff, 0x9d, 0x9b)
		},
		// HUE $0C
		{
			rgb_t(0xae, 0x24, 0x05), rgb_t(0xd2, 0x2d, 0x08),
			rgb_t(0xef, 0x38, 0x0c), rgb_t(0xff, 0x51, 0x24),
			rgb_t(0xff, 0x6f, 0x40), rgb_t(0xff, 0x81, 0x55),
			rgb_t(0xff, 0x93, 0x6f), rgb_t(0xff, 0xab, 0x93)
		},
		// HUE $0D
		{
			rgb_t(0x8c, 0x1d, 0x02), rgb_t(0xb1, 0x29, 0x05),
			rgb_t(0xd0, 0x39, 0x07), rgb_t(0xf9, 0x56, 0x0c),
			rgb_t(0xff, 0x7e, 0x38), rgb_t(0xff, 0x94, 0x4e),
			rgb_t(0xff, 0xa2, 0x63), rgb_t(0xff, 0xb5, 0x86)
		},
		// HUE $0E
		{
			rgb_t(0x65, 0x16, 0x01), rgb_t(0x8a, 0x29, 0x02),
			rgb_t(0xac, 0x41, 0x02), rgb_t(0xd7, 0x68, 0x04),
			rgb_t(0xff, 0x96, 0x1c), rgb_t(0xff, 0xa9, 0x45),
			rgb_t(0xff, 0xb4, 0x57), rgb_t(0xff, 0xc1, 0x79)
		},
		// HUE $0F
		{
			rgb_t(0x3c, 0x0d, 0x00), rgb_t(0x58, 0x21, 0x00),
			rgb_t(0x7d, 0x41, 0x00), rgb_t(0xbb, 0x81, 0x00),
			rgb_t(0xee, 0xb5, 0x00), rgb_t(0xff, 0xc8, 0x20),
			rgb_t(0xff, 0xcc, 0x2e), rgb_t(0xff, 0xcf, 0x48)
		},
		// HUE $10
		{
			rgb_t(0x0c, 0x17, 0x00), rgb_t(0x24, 0x33, 0x00),
			rgb_t(0x49, 0x56, 0x00), rgb_t(0x89, 0x96, 0x00),
			rgb_t(0xbf, 0xc9, 0x00), rgb_t(0xed, 0xef, 0x00),
			rgb_t(0xff, 0xfb, 0x00), rgb_t(0xff, 0xf3, 0x00)
		},
		// HUE $11
		{
			rgb_t(0x0b, 0x2d, 0x00), rgb_t(0x16, 0x48, 0x00),
			rgb_t(0x28, 0x6d, 0x00), rgb_t(0x5e, 0xab, 0x00),
			rgb_t(0x92, 0xdd, 0x00), rgb_t(0xbf, 0xff, 0x00),
			rgb_t(0xcc, 0xff, 0x00), rgb_t(0xd7, 0xff, 0x00)
		},
		// HUE $12
		{
			rgb_t(0x12, 0x3f, 0x00), rgb_t(0x1c, 0x5b, 0x00),
			rgb_t(0x27, 0x7e, 0x00), rgb_t(0x45, 0xbe, 0x00),
			rgb_t(0x6f, 0xee, 0x00), rgb_t(0x8f, 0xff, 0x00),
			rgb_t(0xa1, 0xff, 0x00), rgb_t(0xaf, 0xff, 0x00)
		},
		// HUE $13
		{
			rgb_t(0x17, 0x4e, 0x00), rgb_t(0x21, 0x68, 0x00),
			rgb_t(0x30, 0x8a, 0x00), rgb_t(0x41, 0xc8, 0x00),
			rgb_t(0x59, 0xfa, 0x00), rgb_t(0x6b, 0xff, 0x00),
			rgb_t(0x7d, 0xff, 0x00), rgb_t(0x8b, 0xff, 0x00)
		},
		// HUE $14
		{
			rgb_t(0x1c, 0x59, 0x00), rgb_t(0x25, 0x73, 0x00),
			rgb_t(0x32, 0x94, 0x00), rgb_t(0x43, 0xd1, 0x00),
			rgb_t(0x50, 0xff, 0x00), rgb_t(0x58, 0xff, 0x00),
			rgb_t(0x62, 0xff, 0x00), rgb_t(0x6d, 0xff, 0x00)
		},
		// HUE $15
		{
			rgb_t(0x1d, 0x61, 0x00), rgb_t(0x27, 0x7b, 0x00),
			rgb_t(0x2f, 0x9d, 0x00), rgb_t(0x45, 0xd8, 0x00),
			rgb_t(0x50, 0xff, 0x00), rgb_t(0x4c, 0xff, 0x00),
			rgb_t(0x4e, 0xff, 0x00), rgb_t(0x54, 0xff, 0x00)
		},
		// HUE $16
		{
			rgb_t(0x1f, 0x65, 0x00), rgb_t(0x28, 0x7f, 0x00),
			rgb_t(0x31, 0xa1, 0x00), rgb_t(0x46, 0xdc, 0x00),
			rgb_t(0x51, 0xff, 0x00), rgb_t(0x47, 0xff, 0x00),
			rgb_t(0x3f, 0xff, 0x00), rgb_t(0x3b, 0xff, 0x00)
		},
		// HUE $17
		{
			rgb_t(0x1c, 0x65, 0x00), rgb_t(0x26, 0x7f, 0x00),
			rgb_t(0x32, 0xa0, 0x00), rgb_t(0x46, 0xdb, 0x00),
			rgb_t(0x4c, 0xff, 0x00), rgb_t(0x38, 0xff, 0x00),
			rgb_t(0x0a, 0xff, 0x00), rgb_t(0x00, 0xff, 0x21)
		},
		// HUE $18
		{
			rgb_t(0x1e, 0x61, 0x00), rgb_t(0x26, 0x7c, 0x00),
			rgb_t(0x34, 0x9d, 0x00), rgb_t(0x41, 0xd9, 0x00),
			rgb_t(0x41, 0xff, 0x23), rgb_t(0x20, 0xff, 0x33),
			rgb_t(0x00, 0xff, 0x4b), rgb_t(0x00, 0xff, 0x57)
		},
		// HUE $19
		{
			rgb_t(0x19, 0x59, 0x00), rgb_t(0x26, 0x73, 0x00),
			rgb_t(0x2e, 0x96, 0x00), rgb_t(0x38, 0xd3, 0x2b),
			rgb_t(0x46, 0xff, 0x68), rgb_t(0x2c, 0xff, 0x76),
			rgb_t(0x1c, 0xff, 0x81), rgb_t(0x12, 0xff, 0x8a)
		},
		// HUE $1A
		{
			rgb_t(0x14, 0x4e, 0x00), rgb_t(0x19, 0x69, 0x05),
			rgb_t(0x23, 0x8c, 0x26), rgb_t(0x38, 0xc9, 0x6b),
			rgb_t(0x43, 0xf8, 0xa4), rgb_t(0x42, 0xff, 0xb3),
			rgb_t(0x41, 0xff, 0xb9), rgb_t(0x46, 0xff, 0xbe)
		},
		// HUE $1B
		{
			rgb_t(0x0a, 0x3d, 0x25), rgb_t(0x14, 0x58, 0x40),
			rgb_t(0x20, 0x7c, 0x64), rgb_t(0x30, 0xba, 0xa6),
			rgb_t(0x3e, 0xeb, 0xdc), rgb_t(0x49, 0xff, 0xef),
			rgb_t(0x4d, 0xff, 0xee), rgb_t(0x5d, 0xff, 0xee)
		},
		// HUE $1C
		{
			rgb_t(0x01, 0x29, 0x5c), rgb_t(0x08, 0x44, 0x75),
			rgb_t(0x13, 0x68, 0x9a), rgb_t(0x2b, 0xa8, 0xdc),
			rgb_t(0x4f, 0xd1, 0xff), rgb_t(0x6b, 0xde, 0xff),
			rgb_t(0x7c, 0xe6, 0xff), rgb_t(0x93, 0xec, 0xff)
		},
		// HUE $1D
		{
			rgb_t(0x00, 0x29, 0x7d), rgb_t(0x00, 0x36, 0x9c),
			rgb_t(0x01, 0x52, 0xc5), rgb_t(0x21, 0x91, 0xff),
			rgb_t(0x58, 0xb1, 0xff), rgb_t(0x73, 0xc4, 0xff),
			rgb_t(0x89, 0xd0, 0xff), rgb_t(0xa8, 0xde, 0xff)
		},
		// HUE $1E
		{
			rgb_t(0x00, 0x34, 0x97), rgb_t(0x00, 0x3b, 0xb3),
			rgb_t(0x00, 0x46, 0xdd), rgb_t(0x26, 0x75, 0xff),
			rgb_t(0x54, 0x9b, 0xff), rgb_t(0x72, 0xb0, 0xff),
			rgb_t(0x88, 0xbd, 0xff), rgb_t(0xa7, 0xd0, 0xff)
		},
		// HUE $1F
		{
			rgb_t(0x00, 0x3b, 0xaa), rgb_t(0x00, 0x43, 0xc3),
			rgb_t(0x00, 0x4a, 0xea), rgb_t(0x27, 0x5f, 0xff),
			rgb_t(0x5c, 0x88, 0xff), rgb_t(0x7a, 0x9e, 0xff),
			rgb_t(0x8b, 0xab, 0xff), rgb_t(0xa7, 0xc0, 0xff)
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
