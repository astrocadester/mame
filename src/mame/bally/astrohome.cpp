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

	    Ryland's Eye-One measurement round 2:
		https://groups.io/g/ballyalley/topic/fix_for_7618_astrocde_and/121154924
	*/	
	static constexpr rgb_t colors[32][8] =
	{
		// HUE $00
		{
			rgb_t(0x00, 0x00, 0x00), rgb_t(0x05, 0x09, 0x08),
			rgb_t(0x17, 0x21, 0x1e), rgb_t(0x49, 0x57, 0x54),
			rgb_t(0x8e, 0x99, 0x97), rgb_t(0xc1, 0xc9, 0xc7),
			rgb_t(0xe3, 0xe7, 0xe6), rgb_t(0xff, 0xff, 0xff)
		},
		// HUE $01
		{
			rgb_t(0x2f, 0x31, 0xe2), rgb_t(0x45, 0x37, 0xf9),
			rgb_t(0x55, 0x39, 0xff), rgb_t(0x6d, 0x3b, 0xff),
			rgb_t(0x89, 0x44, 0xff), rgb_t(0x9d, 0x57, 0xff),
			rgb_t(0xa7, 0x64, 0xff), rgb_t(0xb0, 0x73, 0xff)
		},
		// HUE $02
		{
			rgb_t(0x61, 0x32, 0xd5), rgb_t(0x7b, 0x38, 0xec),
			rgb_t(0x91, 0x3f, 0xff), rgb_t(0xa3, 0x3f, 0xff),
			rgb_t(0xb6, 0x45, 0xff), rgb_t(0xc3, 0x51, 0xff),
			rgb_t(0xc7, 0x5b, 0xff), rgb_t(0xcf, 0x6c, 0xff)
		},
		// HUE $03
		{
			rgb_t(0x95, 0x31, 0xbf), rgb_t(0xae, 0x39, 0xd6),
			rgb_t(0xc5, 0x40, 0xec), rgb_t(0xdd, 0x47, 0xff),
			rgb_t(0xe6, 0x49, 0xff), rgb_t(0xea, 0x50, 0xff),
			rgb_t(0xec, 0x57, 0xff), rgb_t(0xee, 0x68, 0xff)
		},
		// HUE $04
		{
			rgb_t(0xc0, 0x33, 0xa1), rgb_t(0xd8, 0x3c, 0xba),
			rgb_t(0xef, 0x42, 0xd0), rgb_t(0xff, 0x46, 0xe2),
			rgb_t(0xff, 0x48, 0xe6), rgb_t(0xff, 0x4c, 0xe9),
			rgb_t(0xff, 0x52, 0xeb), rgb_t(0xff, 0x62, 0xee)
		},
		// HUE $05
		{
			rgb_t(0xe5, 0x33, 0x7d), rgb_t(0xfd, 0x3c, 0x98),
			rgb_t(0xff, 0x3e, 0xa3), rgb_t(0xff, 0x40, 0xae),
			rgb_t(0xff, 0x42, 0xba), rgb_t(0xff, 0x45, 0xc3),
			rgb_t(0xff, 0x4a, 0xc9), rgb_t(0xff, 0x59, 0xd0)
		},
		// HUE $06
		{
			rgb_t(0xff, 0x35, 0x51), rgb_t(0xff, 0x37, 0x68),
			rgb_t(0xff, 0x38, 0x75), rgb_t(0xff, 0x3a, 0x85),
			rgb_t(0xff, 0x3d, 0x97), rgb_t(0xff, 0x40, 0xa5),
			rgb_t(0xff, 0x45, 0xac), rgb_t(0xff, 0x54, 0xb6)
		},
		// HUE $07
		{
			rgb_t(0xff, 0x33, 0x20), rgb_t(0xff, 0x34, 0x3c),
			rgb_t(0xff, 0x36, 0x4e), rgb_t(0xff, 0x37, 0x61),
			rgb_t(0xff, 0x3a, 0x78), rgb_t(0xff, 0x3d, 0x89),
			rgb_t(0xff, 0x42, 0x92), rgb_t(0xff, 0x51, 0x9f)
		},
		// HUE $08
		{
			rgb_t(0xff, 0x32, 0x00), rgb_t(0xff, 0x33, 0x0a),
			rgb_t(0xff, 0x34, 0x25), rgb_t(0xff, 0x35, 0x3c),
			rgb_t(0xff, 0x38, 0x59), rgb_t(0xff, 0x3b, 0x6d),
			rgb_t(0xff, 0x41, 0x79), rgb_t(0xff, 0x51, 0x88)
		},
		// HUE $09
		{
			rgb_t(0xff, 0x33, 0x00), rgb_t(0xff, 0x33, 0x00),
			rgb_t(0xff, 0x33, 0x00), rgb_t(0xff, 0x33, 0x12),
			rgb_t(0xff, 0x34, 0x38), rgb_t(0xff, 0x3c, 0x51),
			rgb_t(0xff, 0x42, 0x5f), rgb_t(0xff, 0x54, 0x71)
		},
		// HUE $0A
		{
			rgb_t(0xff, 0x33, 0x00), rgb_t(0xff, 0x33, 0x00),
			rgb_t(0xff, 0x33, 0x00), rgb_t(0xff, 0x33, 0x00),
			rgb_t(0xff, 0x35, 0x12), rgb_t(0xff, 0x3e, 0x33),
			rgb_t(0xff, 0x48, 0x45), rgb_t(0xff, 0x5a, 0x5b)
		},
		// HUE $0B
		{
			rgb_t(0xf4, 0x30, 0x00), rgb_t(0xff, 0x34, 0x00),
			rgb_t(0xff, 0x35, 0x00), rgb_t(0xff, 0x34, 0x00),
			rgb_t(0xff, 0x38, 0x00), rgb_t(0xff, 0x47, 0x0d),
			rgb_t(0xff, 0x53, 0x27), rgb_t(0xff, 0x65, 0x42)
		},
		// HUE $0C
		{
			rgb_t(0xd6, 0x28, 0x00), rgb_t(0xee, 0x2e, 0x00),
			rgb_t(0xff, 0x34, 0x00), rgb_t(0xff, 0x35, 0x00),
			rgb_t(0xff, 0x44, 0x00), rgb_t(0xff, 0x58, 0x00),
			rgb_t(0xff, 0x65, 0x00), rgb_t(0xff, 0x76, 0x24)
		},
		// HUE $0D
		{
			rgb_t(0xab, 0x20, 0x00), rgb_t(0xc3, 0x26, 0x00),
			rgb_t(0xd9, 0x2d, 0x00), rgb_t(0xfe, 0x40, 0x00),
			rgb_t(0xff, 0x5c, 0x00), rgb_t(0xff, 0x72, 0x00),
			rgb_t(0xff, 0x7e, 0x00), rgb_t(0xff, 0x8e, 0x0d)
		},
		// HUE $0E
		{
			rgb_t(0x76, 0x13, 0x00), rgb_t(0x8e, 0x1d, 0x00),
			rgb_t(0xa5, 0x2d, 0x00), rgb_t(0xcc, 0x51, 0x00),
			rgb_t(0xff, 0x87, 0x00), rgb_t(0xff, 0x9a, 0x00),
			rgb_t(0xff, 0xa3, 0x00), rgb_t(0xff, 0xae, 0x0d)
		},
		// HUE $0F
		{
			rgb_t(0x37, 0x11, 0x00), rgb_t(0x51, 0x29, 0x00),
			rgb_t(0x69, 0x41, 0x00), rgb_t(0x95, 0x6c, 0x00),
			rgb_t(0xd3, 0xa8, 0x00), rgb_t(0xff, 0xd3, 0x00),
			rgb_t(0xff, 0xd6, 0x00), rgb_t(0xff, 0xd9, 0x0d)
		},
		// HUE $10
		{
			rgb_t(0x10, 0x2a, 0x00), rgb_t(0x22, 0x44, 0x00),
			rgb_t(0x36, 0x5d, 0x00), rgb_t(0x5e, 0x89, 0x00),
			rgb_t(0x9d, 0xc5, 0x00), rgb_t(0xd2, 0xf3, 0x00),
			rgb_t(0xe5, 0xff, 0x00), rgb_t(0xec, 0xff, 0x0d)
		},
		// HUE $11
		{
			rgb_t(0x1b, 0x43, 0x00), rgb_t(0x27, 0x5d, 0x00),
			rgb_t(0x33, 0x74, 0x00), rgb_t(0x47, 0xa0, 0x00),
			rgb_t(0x74, 0xdb, 0x00), rgb_t(0x9e, 0xff, 0x00),
			rgb_t(0xab, 0xff, 0x00), rgb_t(0xba, 0xff, 0x0d)
		},
		// HUE $12
		{
			rgb_t(0x24, 0x58, 0x00), rgb_t(0x2f, 0x71, 0x00),
			rgb_t(0x3b, 0x88, 0x00), rgb_t(0x4d, 0xb1, 0x00),
			rgb_t(0x67, 0xeb, 0x00), rgb_t(0x7a, 0xff, 0x00),
			rgb_t(0x86, 0xff, 0x00), rgb_t(0x97, 0xff, 0x0d)
		},
		// HUE $13
		{
			rgb_t(0x2b, 0x67, 0x00), rgb_t(0x37, 0x7f, 0x00),
			rgb_t(0x41, 0x96, 0x00), rgb_t(0x53, 0xbf, 0x00),
			rgb_t(0x6b, 0xf8, 0x00), rgb_t(0x6f, 0xff, 0x00),
			rgb_t(0x73, 0xff, 0x00), rgb_t(0x82, 0xff, 0x0d)
		},
		// HUE $14
		{
			rgb_t(0x30, 0x71, 0x00), rgb_t(0x3c, 0x89, 0x00),
			rgb_t(0x45, 0xa0, 0x00), rgb_t(0x57, 0xc9, 0x00),
			rgb_t(0x6f, 0xff, 0x00), rgb_t(0x6e, 0xff, 0x00),
			rgb_t(0x72, 0xff, 0x00), rgb_t(0x7a, 0xff, 0x0d)
		},
		// HUE $15
		{
			rgb_t(0x33, 0x78, 0x00), rgb_t(0x3d, 0x90, 0x00),
			rgb_t(0x47, 0xa7, 0x00), rgb_t(0x59, 0xcf, 0x00),
			rgb_t(0x70, 0xff, 0x00), rgb_t(0x70, 0xff, 0x00),
			rgb_t(0x6e, 0xff, 0x20), rgb_t(0x7b, 0xff, 0x4d)
		},
		// HUE $16
		{
			rgb_t(0x32, 0x7b, 0x00), rgb_t(0x3f, 0x92, 0x00),
			rgb_t(0x49, 0xaa, 0x00), rgb_t(0x59, 0xd1, 0x00),
			rgb_t(0x6e, 0xff, 0x00), rgb_t(0x6f, 0xff, 0x39),
			rgb_t(0x70, 0xff, 0x56), rgb_t(0x7c, 0xff, 0x72)
		},
		// HUE $17
		{
			rgb_t(0x31, 0x78, 0x00), rgb_t(0x3d, 0x90, 0x00),
			rgb_t(0x48, 0xa7, 0x00), rgb_t(0x5a, 0xce, 0x00),
			rgb_t(0x6e, 0xff, 0x47), rgb_t(0x6e, 0xff, 0x6d),
			rgb_t(0x70, 0xff, 0x80), rgb_t(0x7b, 0xff, 0x95)
		},
		// HUE $18
		{
			rgb_t(0x2f, 0x72, 0x00), rgb_t(0x3b, 0x8a, 0x00),
			rgb_t(0x45, 0xa1, 0x06), rgb_t(0x56, 0xc8, 0x40),
			rgb_t(0x6e, 0xfd, 0x85), rgb_t(0x6f, 0xff, 0x9e),
			rgb_t(0x71, 0xff, 0xaa), rgb_t(0x7b, 0xff, 0xb8)
		},
		// HUE $19
		{
			rgb_t(0x29, 0x66, 0x11), rgb_t(0x35, 0x7e, 0x33),
			rgb_t(0x40, 0x96, 0x4f), rgb_t(0x52, 0xbd, 0x7d),
			rgb_t(0x6c, 0xf2, 0xbc), rgb_t(0x71, 0xff, 0xd1),
			rgb_t(0x72, 0xff, 0xd7), rgb_t(0x7d, 0xff, 0xde)
		},
		// HUE $1A
		{
			rgb_t(0x22, 0x56, 0x4b), rgb_t(0x2e, 0x6f, 0x69),
			rgb_t(0x3a, 0x85, 0x82), rgb_t(0x4e, 0xaf, 0xb0),
			rgb_t(0x69, 0xe5, 0xeb), rgb_t(0x71, 0xf9, 0xff),
			rgb_t(0x73, 0xf9, 0xff), rgb_t(0x7c, 0xfa, 0xff)
		},
		// HUE $1B
		{
			rgb_t(0x1b, 0x43, 0x7c), rgb_t(0x28, 0x5b, 0x96),
			rgb_t(0x34, 0x72, 0xaf), rgb_t(0x49, 0x9c, 0xdb),
			rgb_t(0x5c, 0xc4, 0xff), rgb_t(0x60, 0xce, 0xff),
			rgb_t(0x64, 0xd3, 0xff), rgb_t(0x6f, 0xdb, 0xff)
		},
		// HUE $1C
		{
			rgb_t(0x16, 0x32, 0xa3), rgb_t(0x22, 0x47, 0xbb),
			rgb_t(0x30, 0x5c, 0xd3), rgb_t(0x45, 0x87, 0xfe),
			rgb_t(0x4e, 0x9e, 0xff), rgb_t(0x55, 0xad, 0xff),
			rgb_t(0x5a, 0xb7, 0xff), rgb_t(0x67, 0xc3, 0xff)
		},
		// HUE $1D
		{
			rgb_t(0x1a, 0x2b, 0xc1), rgb_t(0x23, 0x39, 0xd7),
			rgb_t(0x2c, 0x4a, 0xef), rgb_t(0x39, 0x66, 0xff),
			rgb_t(0x45, 0x81, 0xff), rgb_t(0x4d, 0x95, 0xff),
			rgb_t(0x55, 0xa1, 0xff), rgb_t(0x64, 0xb1, 0xff)
		},
		// HUE $1E
		{
			rgb_t(0x1e, 0x2e, 0xd6), rgb_t(0x24, 0x36, 0xec),
			rgb_t(0x2e, 0x3d, 0xff), rgb_t(0x34, 0x4e, 0xff),
			rgb_t(0x3e, 0x69, 0xff), rgb_t(0x4c, 0x81, 0xff),
			rgb_t(0x59, 0x90, 0xff), rgb_t(0x6b, 0x9f, 0xff)
		},
		// HUE $1F
		{
			rgb_t(0x22, 0x32, 0xe3), rgb_t(0x2b, 0x36, 0xf8),
			rgb_t(0x2c, 0x39, 0xff), rgb_t(0x32, 0x3f, 0xff),
			rgb_t(0x43, 0x57, 0xff), rgb_t(0x5a, 0x6f, 0xff),
			rgb_t(0x6b, 0x80, 0xff), rgb_t(0x7c, 0x8e, 0xff)
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
