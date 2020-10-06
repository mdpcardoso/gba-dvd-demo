
#include <tonc_bios.h>
#include <tonc_input.h>
#include <tonc_irq.h>
#include <tonc_memdef.h>
#include <tonc_memmap.h>
#include <tonc_oam.h>
#include <tonc_types.h>
#include <tonc_video.h>

#include "sprite.h"

OBJ_ATTR obj_buffer[1];
OBJ_AFFINE *const obj_aff_buffer = (OBJ_AFFINE *)obj_buffer;

struct sprite_t {
  u32 width, height;
  u32 x, y;
  s32 x_speed, y_speed;
  u32 pal_bank;
  OBJ_ATTR *oam_buffer;
};

void cycle_sprite_pal(struct sprite_t *const sprite) {
  if (sprite->pal_bank < 4) {
    sprite->pal_bank++;
  } else {
    sprite->pal_bank = 0;
  }

  sprite->oam_buffer->attr2 = ATTR2_PALBANK(sprite->pal_bank) | 1;
}
//---------------------------------------------------------------------------------
// Program entry point
//---------------------------------------------------------------------------------
int main(void) {
  //---------------------------------------------------------------------------------
  // Check if sizes are half-words. (Verify bot memcpy16)
  memcpy16(pal_obj_mem, spritePal, spritePalLen / 2);
  memcpy16(&tile_mem[4][1], spriteTiles, spriteTilesLen / 2);
  // Need to memset bg even if disabled?
  memset16(pal_bg_mem, 0, PAL_BG_SIZE / 2);

  oam_init(obj_buffer, 1);

  struct sprite_t dvd = {64, 32, 0, 0, 1, 1, 0, &obj_buffer[0]};

  dvd.oam_buffer->attr0 =
      ATTR0_4BPP | ATTR0_SHAPE(ATTR0_WIDE) |
      ATTR0_Y(dvd.y); // 4bpp tiles, WIDE shape, at y coord 0
  dvd.oam_buffer->attr1 =
      ATTR1_SIZE_64x32 |
      ATTR1_X(dvd.x); // 64x32 size when using the WIDE shape
  dvd.oam_buffer->attr2 =
      ATTR2_PALBANK(dvd.pal_bank) | 1; // Start at [4][1] (4-bit spacing)

  REG_DISPCNT = DCNT_MODE0 | DCNT_OBJ | DCNT_OBJ_1D; // Mode 0, no background

  // the vblank interrupt must be enabled for VBlankIntrWait() to work
  // since the default dispatcher handles the bios flags no vblank handler
  // is required
  IRQ_INIT();
  IRQ_SET(VBLANK);

  while (1) {
    key_poll();

    obj_set_pos(dvd.oam_buffer, dvd.x += dvd.x_speed, dvd.y += dvd.y_speed);

    if (dvd.x + dvd.width >= SCREEN_WIDTH || dvd.x <= 0) {
      dvd.x_speed *= -1;

      cycle_sprite_pal(&dvd);
    }

    if (dvd.y + dvd.height >= SCREEN_HEIGHT || dvd.y <= 0) {
      dvd.y_speed *= -1;

      cycle_sprite_pal(&dvd);
    }

    VBlankIntrWait();

    oam_copy(oam_mem, obj_buffer, 1); // Buffer the OAM copy
  }
}
