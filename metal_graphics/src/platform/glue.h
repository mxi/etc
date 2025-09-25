/* A set of application hooks called from the platform layer.  */
#ifndef MXI_METAL_GRAPHICS_GLUE_H
#define MXI_METAL_GRAPHICS_GLUE_H 1

extern void mg_init(void) __attribute__((weak));

extern void mg_shutdown(void) __attribute__((weak));

extern void mg_draw(void) __attribute__((weak));

#endif /* MXI_METAL_GRAPHICS_GLUE_H */