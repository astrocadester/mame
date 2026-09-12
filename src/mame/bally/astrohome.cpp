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

	    Ryland's Eye-One measurements:
		https://groups.io/g/ballyalley/topic/fix_for_7618_astrocde_and/121154924
	*/	
	static constexpr rgb_t colors[32][8] =
	{
		// HUE $00
		{
			rgb_t(0x00, 0x00, 0x00), rgb_t(0x01, 0x05, 0x03),
			rgb_t(0x0c, 0x17, 0x14), rgb_t(0x3c, 0x4b, 0x47),
			rgb_t(0x87, 0x92, 0x90), rgb_t(0xbe, 0xc5, 0xc4),
			rgb_t(0xe2, 0xe6, 0xe5), rgb_t(0xff, 0xff, 0xff)
		},
		// HUE $01
		{
			rgb_t(0x29, 0x30, 0xe0), rgb_t(0x3f, 0x34, 0xf8),
			rgb_t(0x4e, 0x37, 0xff), rgb_t(0x66, 0x37, 0xff),
			rgb_t(0x83, 0x3e, 0xff), rgb_t(0x96, 0x4f, 0xff),
			rgb_t(0xa3, 0x5d, 0xff), rgb_t(0xb0, 0x70, 0xff)
		},
		// HUE $02
		{
			rgb_t(0x5b, 0x2f, 0xd5), rgb_t(0x76, 0x36, 0xed),
			rgb_t(0x8b, 0x3c, 0xff), rgb_t(0x9c, 0x3c, 0xff),
			rgb_t(0xb1, 0x40, 0xff), rgb_t(0xbf, 0x4b, 0xff),
			rgb_t(0xc5, 0x55, 0xff), rgb_t(0xcd, 0x68, 0xff)
		},
		// HUE $03
		{
			rgb_t(0x8f, 0x2f, 0xbe), rgb_t(0xa8, 0x38, 0xd7),
			rgb_t(0xbf, 0x3f, 0xec), rgb_t(0xd8, 0x44, 0xff),
			rgb_t(0xe1, 0x45, 0xff), rgb_t(0xe7, 0x4b, 0xff),
			rgb_t(0xea, 0x53, 0xff), rgb_t(0xed, 0x63, 0xff)
		},
		// HUE $04
		{
			rgb_t(0xbd, 0x30, 0xa1), rgb_t(0xd7, 0x39, 0xbb),
			rgb_t(0xee, 0x40, 0xd1), rgb_t(0xff, 0x45, 0xe4),
			rgb_t(0xff, 0x46, 0xe8), rgb_t(0xff, 0x49, 0xeb),
			rgb_t(0xff, 0x4e, 0xec), rgb_t(0xff, 0x5e, 0xef)
		},
		// HUE $05
		{
			rgb_t(0xe4, 0x33, 0x7b), rgb_t(0xfe, 0x3b, 0x98),
			rgb_t(0xff, 0x3d, 0xa2), rgb_t(0xff, 0x3e, 0xad),
			rgb_t(0xff, 0x3f, 0xba), rgb_t(0xff, 0x42, 0xc3),
			rgb_t(0xff, 0x47, 0xc8), rgb_t(0xff, 0x56, 0xcf)
		},
		// HUE $06
		{
			rgb_t(0xff, 0x34, 0x4e), rgb_t(0xff, 0x35, 0x64),
			rgb_t(0xff, 0x36, 0x72), rgb_t(0xff, 0x39, 0x82),
			rgb_t(0xff, 0x3a, 0x96), rgb_t(0xff, 0x3e, 0xa3),
			rgb_t(0xff, 0x42, 0xaa), rgb_t(0xff, 0x51, 0xb5)
		},
		// HUE $07
		{
			rgb_t(0xff, 0x32, 0x1a), rgb_t(0xff, 0x33, 0x37),
			rgb_t(0xff, 0x34, 0x49), rgb_t(0xff, 0x35, 0x5c),
			rgb_t(0xff, 0x38, 0x75), rgb_t(0xff, 0x3b, 0x87),
			rgb_t(0xff, 0x3f, 0x90), rgb_t(0xff, 0x4e, 0x9d)
		},
		// HUE $08
		{
			rgb_t(0xff, 0x32, 0x00), rgb_t(0xff, 0x31, 0x01),
			rgb_t(0xff, 0x32, 0x1f), rgb_t(0xff, 0x33, 0x36),
			rgb_t(0xff, 0x36, 0x55), rgb_t(0xff, 0x38, 0x6a),
			rgb_t(0xff, 0x3f, 0x76), rgb_t(0xff, 0x4e, 0x85)
		},
		// HUE $09
		{
			rgb_t(0xff, 0x31, 0x00), rgb_t(0xff, 0x32, 0x00),
			rgb_t(0xff, 0x32, 0x00), rgb_t(0xff, 0x33, 0x07),
			rgb_t(0xff, 0x34, 0x32), rgb_t(0xff, 0x39, 0x4d),
			rgb_t(0xff, 0x40, 0x5c), rgb_t(0xff, 0x51, 0x6f)
		},
		// HUE $0A
		{
			rgb_t(0xff, 0x32, 0x00), rgb_t(0xff, 0x33, 0x00),
			rgb_t(0xff, 0x32, 0x00), rgb_t(0xff, 0x31, 0x00),
			rgb_t(0xff, 0x33, 0x09), rgb_t(0xff, 0x3b, 0x2e),
			rgb_t(0xff, 0x45, 0x41), rgb_t(0xff, 0x58, 0x57)
		},
		// HUE $0B
		{
			rgb_t(0xf8, 0x30, 0x00), rgb_t(0xff, 0x32, 0x00),
			rgb_t(0xff, 0x32, 0x00), rgb_t(0xff, 0x31, 0x00),
			rgb_t(0xff, 0x36, 0x00), rgb_t(0xff, 0x44, 0x03),
			rgb_t(0xff, 0x50, 0x21), rgb_t(0xff, 0x63, 0x3d)
		},
		// HUE $0C
		{
			rgb_t(0xd7, 0x2a, 0x00), rgb_t(0xf1, 0x2f, 0x00),
			rgb_t(0xff, 0x34, 0x00), rgb_t(0xff, 0x33, 0x00),
			rgb_t(0xff, 0x40, 0x00), rgb_t(0xff, 0x54, 0x00),
			rgb_t(0xff, 0x61, 0x00), rgb_t(0xff, 0x74, 0x1c)
		},
		// HUE $0D
		{
			rgb_t(0xac, 0x20, 0x00), rgb_t(0xc5, 0x24, 0x00),
			rgb_t(0xdd, 0x2c, 0x00), rgb_t(0xff, 0x3d, 0x00),
			rgb_t(0xff, 0x58, 0x00), rgb_t(0xff, 0x6f, 0x00),
			rgb_t(0xff, 0x7c, 0x00), rgb_t(0xff, 0x8d, 0x0d)
		},
		// HUE $0E
		{
			rgb_t(0x75, 0x13, 0x00), rgb_t(0x8f, 0x1d, 0x00),
			rgb_t(0xa7, 0x2d, 0x00), rgb_t(0xd2, 0x4f, 0x00),
			rgb_t(0xff, 0x85, 0x00), rgb_t(0xff, 0x98, 0x00),
			rgb_t(0xff, 0xa1, 0x00), rgb_t(0xff, 0xac, 0x0d)
		},
		// HUE $0F
		{
			rgb_t(0x32, 0x0f, 0x00), rgb_t(0x4d, 0x27, 0x00),
			rgb_t(0x68, 0x3f, 0x00), rgb_t(0x96, 0x6c, 0x00),
			rgb_t(0xda, 0xad, 0x00), rgb_t(0xff, 0xd2, 0x00),
			rgb_t(0xff, 0xd4, 0x00), rgb_t(0xff, 0xd8, 0x0d)
		},
		// HUE $10
		{
			rgb_t(0x0e, 0x27, 0x00), rgb_t(0x20, 0x42, 0x00),
			rgb_t(0x34, 0x5c, 0x00), rgb_t(0x5f, 0x8a, 0x00),
			rgb_t(0xa2, 0xcb, 0x00), rgb_t(0xda, 0xfd, 0x00),
			rgb_t(0xe5, 0xff, 0x00), rgb_t(0xec, 0xff, 0x0d)
		},
		// HUE $11
		{
			rgb_t(0x1a, 0x42, 0x00), rgb_t(0x28, 0x5c, 0x00),
			rgb_t(0x34, 0x77, 0x00), rgb_t(0x4c, 0xa5, 0x00),
			rgb_t(0x7a, 0xe6, 0x00), rgb_t(0x9e, 0xff, 0x00),
			rgb_t(0xad, 0xff, 0x00), rgb_t(0xbc, 0xff, 0x0d)
		},
		// HUE $12
		{
			rgb_t(0x25, 0x59, 0x00), rgb_t(0x33, 0x72, 0x00),
			rgb_t(0x3e, 0x8b, 0x00), rgb_t(0x53, 0xb9, 0x00),
			rgb_t(0x6f, 0xf6, 0x00), rgb_t(0x7c, 0xff, 0x00),
			rgb_t(0x87, 0xff, 0x00), rgb_t(0x97, 0xff, 0x0d)
		},
		// HUE $13
		{
			rgb_t(0x2d, 0x69, 0x00), rgb_t(0x3a, 0x83, 0x00),
			rgb_t(0x44, 0x9b, 0x00), rgb_t(0x59, 0xc6, 0x00),
			rgb_t(0x73, 0xff, 0x00), rgb_t(0x73, 0xff, 0x00),
			rgb_t(0x75, 0xff, 0x00), rgb_t(0x85, 0xff, 0x0d)
		},
		// HUE $14
		{
			rgb_t(0x31, 0x75, 0x00), rgb_t(0x3d, 0x8f, 0x00),
			rgb_t(0x4b, 0xa6, 0x00), rgb_t(0x5f, 0xd2, 0x00),
			rgb_t(0x73, 0xff, 0x00), rgb_t(0x72, 0xff, 0x00),
			rgb_t(0x72, 0xff, 0x00), rgb_t(0x7d, 0xff, 0x0d)
		},
		// HUE $15
		{
			rgb_t(0x38, 0x7c, 0x00), rgb_t(0x42, 0x96, 0x00),
			rgb_t(0x4e, 0xae, 0x00), rgb_t(0x62, 0xd9, 0x00),
			rgb_t(0x73, 0xff, 0x00), rgb_t(0x70, 0xff, 0x00),
			rgb_t(0x73, 0xff, 0x16), rgb_t(0x7c, 0xff, 0x47)
		},
		// HUE $16
		{
			rgb_t(0x38, 0x7f, 0x00), rgb_t(0x43, 0x99, 0x00),
			rgb_t(0x4f, 0xb1, 0x00), rgb_t(0x63, 0xda, 0x00),
			rgb_t(0x72, 0xff, 0x00), rgb_t(0x72, 0xff, 0x32),
			rgb_t(0x74, 0xff, 0x51), rgb_t(0x7e, 0xff, 0x6e)
		},
		// HUE $17
		{
			rgb_t(0x36, 0x7c, 0x00), rgb_t(0x43, 0x96, 0x00),
			rgb_t(0x4d, 0xae, 0x00), rgb_t(0x61, 0xd9, 0x00),
			rgb_t(0x72, 0xff, 0x40), rgb_t(0x72, 0xff, 0x69),
			rgb_t(0x73, 0xff, 0x7c), rgb_t(0x7b, 0xff, 0x90)
		},
		// HUE $18
		{
			rgb_t(0x32, 0x76, 0x00), rgb_t(0x40, 0x90, 0x00),
			rgb_t(0x4a, 0xa9, 0x00), rgb_t(0x5d, 0xd1, 0x3d),
			rgb_t(0x74, 0xff, 0x82), rgb_t(0x72, 0xff, 0x9a),
			rgb_t(0x75, 0xff, 0xa7), rgb_t(0x7f, 0xff, 0xb5)
		},
		// HUE $19
		{
			rgb_t(0x2d, 0x69, 0x0c), rgb_t(0x38, 0x84, 0x30),
			rgb_t(0x46, 0x9c, 0x4e), rgb_t(0x58, 0xc6, 0x7f),
			rgb_t(0x74, 0xff, 0xc2), rgb_t(0x75, 0xff, 0xce),
			rgb_t(0x76, 0xff, 0xd4), rgb_t(0x81, 0xff, 0xdc)
		},
		// HUE $1A
		{
			rgb_t(0x25, 0x59, 0x4a), rgb_t(0x30, 0x74, 0x69),
			rgb_t(0x3d, 0x8b, 0x85), rgb_t(0x53, 0xb5, 0xb4),
			rgb_t(0x71, 0xf0, 0xf3), rgb_t(0x78, 0xfb, 0xff),
			rgb_t(0x78, 0xfc, 0xff), rgb_t(0x81, 0xfb, 0xff)
		},
		// HUE $1B
		{
			rgb_t(0x1d, 0x45, 0x7d), rgb_t(0x2a, 0x5e, 0x9a),
			rgb_t(0x36, 0x76, 0xb5), rgb_t(0x4f, 0xa2, 0xe2),
			rgb_t(0x5f, 0xc5, 0xff), rgb_t(0x65, 0xce, 0xff),
			rgb_t(0x68, 0xd4, 0xff), rgb_t(0x73, 0xda, 0xff)
		},
		// HUE $1C
		{
			rgb_t(0x18, 0x33, 0xa7), rgb_t(0x26, 0x49, 0xc1),
			rgb_t(0x33, 0x5f, 0xda), rgb_t(0x45, 0x87, 0xff),
			rgb_t(0x51, 0x9d, 0xff), rgb_t(0x59, 0xac, 0xff),
			rgb_t(0x5d, 0xb5, 0xff), rgb_t(0x69, 0xc0, 0xff)
		},
		// HUE $1D
		{
			rgb_t(0x1c, 0x2c, 0xc8), rgb_t(0x26, 0x39, 0xe0),
			rgb_t(0x30, 0x4b, 0xf8), rgb_t(0x3a, 0x63, 0xff),
			rgb_t(0x48, 0x7e, 0xff), rgb_t(0x4f, 0x92, 0xff),
			rgb_t(0x56, 0x9f, 0xff), rgb_t(0x65, 0xae, 0xff)
		},
		// HUE $1E
		{
			rgb_t(0x22, 0x2f, 0xdf), rgb_t(0x2b, 0x36, 0xf6),
			rgb_t(0x2e, 0x3b, 0xff), rgb_t(0x35, 0x4a, 0xff),
			rgb_t(0x40, 0x66, 0xff), rgb_t(0x4d, 0x7d, 0xff),
			rgb_t(0x59, 0x8d, 0xff), rgb_t(0x6c, 0x9d, 0xff)
		},
		// HUE $1F
		{
			rgb_t(0x27, 0x33, 0xef), rgb_t(0x2c, 0x37, 0xff),
			rgb_t(0x2f, 0x37, 0xff), rgb_t(0x34, 0x3c, 0xff),
			rgb_t(0x43, 0x53, 0xff), rgb_t(0x5a, 0x6b, 0xff),
			rgb_t(0x6b, 0x7d, 0xff), rgb_t(0x7e, 0x8e, 0xff)
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
