#define _GNU_SOURCE
#include "../../include/hal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <drm/drm.h>
#include <drm/drm_mode.h>

/* DRM device path */
#define DRM_DEVICE "/dev/dri/card0"

/* DRM connection states (since DRM_MODE_CONNECTED not available) */
#define DRM_MODE_DISCONNECTED   0
#define DRM_MODE_CONNECTED     1
#define DRM_MODE_UNKNOWNCONNECTION 2

/* Internal state */
static bool lcd_initialized = false;
static int drm_fd = -1;
static uint32_t *fb_buffer = NULL;
static struct drm_mode_create_dumb create_req;
static struct drm_mode_map_dumb map_req;
static uint32_t fb_id;
static size_t buffer_size = 0;

/* Display configuration */
static uint32_t connector_id = 0;
static uint32_t crtc_id = 0;
static struct drm_mode_modeinfo mode;

#if 1
hal_lcd_status_t hal_lcd_init(void)
{
    if (lcd_initialized) {
        return HAL_LCD_OK;
    }

    printf("Initializing LCD via DRM...\n");

    /* Open DRM device */
    drm_fd = open(DRM_DEVICE, O_RDWR);
    if (drm_fd < 0) {
        printf("Error: Cannot open DRM device %s: %s\n", DRM_DEVICE, strerror(errno));
        return HAL_LCD_ERROR;
    }

    /* First, get the count of resources */
    struct drm_mode_card_res resources = {0};
    if (ioctl(drm_fd, DRM_IOCTL_MODE_GETRESOURCES, &resources) < 0) {
        printf("Error: Cannot get DRM resource counts: %s\n", strerror(errno));
        close(drm_fd);
        return HAL_LCD_ERROR;
    }

    printf("DRM Resources: %d connectors, %d crtcs, %d encoders, %d fbs\n", 
           resources.count_connectors, resources.count_crtcs, 
           resources.count_encoders, resources.count_fbs);

    if (resources.count_connectors == 0 || resources.count_crtcs == 0) {
        printf("Error: Insufficient DRM resources\n");
        close(drm_fd);
        return HAL_LCD_ERROR;
    }

    /* Allocate arrays for resource IDs */
    uint32_t *connectors = malloc(resources.count_connectors * sizeof(uint32_t));
    uint32_t *crtcs = malloc(resources.count_crtcs * sizeof(uint32_t));
    uint32_t *encoders = malloc(resources.count_encoders * sizeof(uint32_t));
    uint32_t *fbs = malloc(resources.count_fbs * sizeof(uint32_t));
    
    if (!connectors || !crtcs || !encoders || !fbs) {
        printf("Error: Cannot allocate memory for DRM resources\n");
        free(connectors);
        free(crtcs);
        free(encoders);
        free(fbs);
        close(drm_fd);
        return HAL_LCD_ERROR;
    }

    /* Set up pointers for the second call */
    resources.connector_id_ptr = (uint64_t)connectors;
    resources.crtc_id_ptr = (uint64_t)crtcs;
    resources.encoder_id_ptr = (uint64_t)encoders;
    resources.fb_id_ptr = (uint64_t)fbs;

    /* Second call to get actual resource IDs */
    if (ioctl(drm_fd, DRM_IOCTL_MODE_GETRESOURCES, &resources) < 0) {
        printf("Error: Cannot get DRM resource IDs: %s\n", strerror(errno));
        free(connectors);
        free(crtcs);
        free(encoders);
        free(fbs);
        close(drm_fd);
        return HAL_LCD_ERROR;
    }

    printf("Found %d connectors, %d CRTCs\n", resources.count_connectors, resources.count_crtcs);

    /* Use first CRTC */
    crtc_id = crtcs[0];
    printf("Using CRTC ID: %d\n", crtc_id);

    /* Find a connected connector with modes */
    bool found_connector = false;
    for (int i = 0; i < resources.count_connectors; i++) {
        struct drm_mode_get_connector conn = {0};
        conn.connector_id = connectors[i];
        
        /* First call to get connector info and mode count */
        if (ioctl(drm_fd, DRM_IOCTL_MODE_GETCONNECTOR, &conn) == 0) {
            printf("Connector %d: type=%d, connection=%d, modes=%d\n", 
                   connectors[i], conn.connector_type, conn.connection, conn.count_modes);
                   
            /* Use connected connector with available modes */
            if (conn.connection == DRM_MODE_CONNECTED && conn.count_modes > 0) {
                connector_id = connectors[i];
                printf("Using connected connector ID: %d\n", connector_id);
                
                /* Allocate space for modes and get them */
                struct drm_mode_modeinfo *modes = malloc(conn.count_modes * sizeof(struct drm_mode_modeinfo));
                if (modes) {
                    /* Clear the connector structure and set it up for second call */
                    memset(&conn, 0, sizeof(conn));
                    conn.connector_id = connectors[i];
                    conn.modes_ptr = (uint64_t)modes;
                    
                    /* Second call to get actual modes */
                    if (ioctl(drm_fd, DRM_IOCTL_MODE_GETCONNECTOR, &conn) == 0 && conn.count_modes > 0) {
                        mode = modes[0];  /* Use first mode (usually preferred) */
                        found_connector = true;
                        printf("Using display mode: %dx%d@%dHz\n", 
                               mode.hdisplay, mode.vdisplay, mode.vrefresh);
                        printf("Mode details: hsync_start=%d, hsync_end=%d, htotal=%d\n",
                               mode.hsync_start, mode.hsync_end, mode.htotal);
                        printf("Mode details: vsync_start=%d, vsync_end=%d, vtotal=%d\n",
                               mode.vsync_start, mode.vsync_end, mode.vtotal);
                        printf("Mode clock: %d, flags: 0x%x\n", mode.clock, mode.flags);
                    } else {
                        printf("Error: Failed to get modes for connector %d\n", connectors[i]);
                    }
                    
                    free(modes);
                }
                
                if (found_connector) break;
            }
        }
    }

    /* If no connected connector found, try any connector with modes */
    if (!found_connector) {
        printf("No connected connector found, trying fallback...\n");
        for (int i = 0; i < resources.count_connectors; i++) {
            struct drm_mode_get_connector conn = {0};
            conn.connector_id = connectors[i];
            
            if (ioctl(drm_fd, DRM_IOCTL_MODE_GETCONNECTOR, &conn) == 0) {
                if (conn.count_modes > 0) {
                    connector_id = connectors[i];
                    printf("Using fallback connector ID: %d (not connected but has modes)\n", connector_id);
                    
                    struct drm_mode_modeinfo *modes = malloc(conn.count_modes * sizeof(struct drm_mode_modeinfo));
                    if (modes) {
                        memset(&conn, 0, sizeof(conn));
                        conn.connector_id = connectors[i];
                        conn.modes_ptr = (uint64_t)modes;
                        
                        if (ioctl(drm_fd, DRM_IOCTL_MODE_GETCONNECTOR, &conn) == 0 && conn.count_modes > 0) {
                            mode = modes[0];
                            found_connector = true;
                            printf("Using display mode: %dx%d@%dHz\n", 
                                   mode.hdisplay, mode.vdisplay, mode.vrefresh);
                        }
                        
                        free(modes);
                    }
                    
                    if (found_connector) break;
                }
            }
        }
    }

    /* If still no mode found, use hardcoded values */
    if (!found_connector || mode.hdisplay == 0 || mode.vdisplay == 0) {
        printf("Warning: No valid mode found, using hardcoded 480x800@50Hz\n");
        memset(&mode, 0, sizeof(mode));
        mode.hdisplay = 480;
        mode.vdisplay = 800;
        mode.vrefresh = 50;
        mode.hsync_start = 578;
        mode.hsync_end = 610;
        mode.htotal = 708;
        mode.vsync_start = 815;
        mode.vsync_end = 825;
        mode.vtotal = 839;
        mode.clock = 29700;
        strcpy(mode.name, "480x800");
        
        /* Use the first connector if none was selected */
        if (connector_id == 0 && resources.count_connectors > 0) {
            connector_id = connectors[0];
        }
        
        found_connector = true;
        printf("Using hardcoded mode: %dx%d@%dHz\n", 
               mode.hdisplay, mode.vdisplay, mode.vrefresh);
    }

    printf("DEBUG: Mode before buffer creation: %dx%d\n", mode.hdisplay, mode.vdisplay);
    
    /* Validate mode dimensions */
    if (mode.hdisplay == 0 || mode.vdisplay == 0) {
        printf("ERROR: Invalid mode dimensions, using fallback\n");
        mode.hdisplay = 480;
        mode.vdisplay = 800;
    }

    /* Create dumb buffer for framebuffer using actual display mode */
    memset(&create_req, 0, sizeof(create_req));
    create_req.width = mode.hdisplay;   /* Use actual display width */
    create_req.height = mode.vdisplay;  /* Use actual display height */
    create_req.bpp = LCD_BPP;

    printf("Creating buffer: %dx%d@%dbpp\n", create_req.width, create_req.height, create_req.bpp);
    printf("DEBUG: create_req sizes: width=%u, height=%u, bpp=%u\n", 
           create_req.width, create_req.height, create_req.bpp);

    if (ioctl(drm_fd, DRM_IOCTL_MODE_CREATE_DUMB, &create_req) < 0) {
        printf("Error: Cannot create dumb buffer: %s\n", strerror(errno));
        close(drm_fd);
        drm_fd = -1;
        return HAL_LCD_ERROR;
    }

    printf("Created dumb buffer: handle=%u, pitch=%u, size=%llu\n", 
           create_req.handle, create_req.pitch, create_req.size);

    /* Create framebuffer object using actual mode dimensions */
    struct drm_mode_fb_cmd fb_cmd = {0};
    fb_cmd.width = mode.hdisplay;
    fb_cmd.height = mode.vdisplay;
    fb_cmd.pitch = create_req.pitch;
    fb_cmd.bpp = LCD_BPP;
    fb_cmd.depth = 24;
    fb_cmd.handle = create_req.handle;

    if (ioctl(drm_fd, DRM_IOCTL_MODE_ADDFB, &fb_cmd) < 0) {
        printf("Warning: Cannot create framebuffer object: %s\n", strerror(errno));
        printf("Continuing without framebuffer object...\n");
        fb_id = 0;
    } else {
        fb_id = fb_cmd.fb_id;
        printf("Created framebuffer: ID=%u\n", fb_id);
    }

    /* Map the buffer */
    memset(&map_req, 0, sizeof(map_req));
    map_req.handle = create_req.handle;

    if (ioctl(drm_fd, DRM_IOCTL_MODE_MAP_DUMB, &map_req) < 0) {
        printf("Error: Cannot map dumb buffer: %s\n", strerror(errno));
        if (fb_id) ioctl(drm_fd, DRM_IOCTL_MODE_RMFB, &fb_id);
        struct drm_mode_destroy_dumb destroy_req = { .handle = create_req.handle };
        ioctl(drm_fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy_req);
        close(drm_fd);
        drm_fd = -1;
        return HAL_LCD_ERROR;
    }

    printf("Map buffer: offset=%llu\n", map_req.offset);

    buffer_size = create_req.size;
    fb_buffer = (uint32_t *)mmap(0, buffer_size, PROT_READ | PROT_WRITE, MAP_SHARED, drm_fd, map_req.offset);
    
    if (fb_buffer == MAP_FAILED) {
        printf("Error: Cannot mmap buffer: %s\n", strerror(errno));
        if (fb_id) ioctl(drm_fd, DRM_IOCTL_MODE_RMFB, &fb_id);
        struct drm_mode_destroy_dumb destroy_req = { .handle = create_req.handle };
        ioctl(drm_fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy_req);
        close(drm_fd);
        drm_fd = -1;
        return HAL_LCD_ERROR;
    }

    /* Try to set the display mode (this may fail but we continue) */
    if (fb_id > 0) {
        struct drm_mode_crtc crtc = {0};
        crtc.crtc_id = crtc_id;
        crtc.fb_id = fb_id;
        crtc.set_connectors_ptr = (uint64_t)&connector_id;
        crtc.count_connectors = 1;
        crtc.mode = mode;
        crtc.mode_valid = 1;

        if (ioctl(drm_fd, DRM_IOCTL_MODE_SETCRTC, &crtc) < 0) {
            printf("Warning: Cannot set CRTC mode: %s\n", strerror(errno));
            printf("Display might not show on screen\n");
        } else {
            printf("Successfully set display mode\n");
        }
    }

    lcd_initialized = true;
    printf("LCD DRM initialized successfully (%dx%d, %d bpp, buffer size: %zu)\n", 
           LCD_WIDTH, LCD_HEIGHT, LCD_BPP, buffer_size);

    /* Clear screen to red to test */
    hal_lcd_clear(LCD_COLOR_RED);

    return HAL_LCD_OK;
}
#else
hal_lcd_status_t hal_lcd_init(void)
{
    if (lcd_initialized) {
        return HAL_LCD_OK;
    }

    printf("Initializing LCD via DRM...\n");

    /* Open DRM device */
    drm_fd = open(DRM_DEVICE, O_RDWR);
    if (drm_fd < 0) {
        printf("Error: Cannot open DRM device %s: %s\n", DRM_DEVICE, strerror(errno));
        return HAL_LCD_ERROR;
    }

    /* Use known working mode from modetest output */
    memset(&mode, 0, sizeof(mode));
    mode.hdisplay = 480;        /* Horizontal display size */
    mode.vdisplay = 800;        /* Vertical display size */
    mode.vrefresh = 60;         /* Refresh rate in Hz */
    mode.hsync_start = 578;     /* Horizontal sync start */
    mode.hsync_end = 610;       /* Horizontal sync end */
    mode.htotal = 708;          /* Total horizontal pixels */
    mode.vsync_start = 815;     /* Vertical sync start */
    mode.vsync_end = 825;       /* Vertical sync end */
    mode.vtotal = 839;          /* Total vertical lines */
    mode.clock = 29700;         /* Pixel clock in kHz */
    mode.flags = 0;             /* Mode flags (nhsync, nvsync) */
    strcpy(mode.name, "480x800");
    
    /* Use DSI connector and CRTC directly from modetest output */
    connector_id = 34;          /* DSI-1 connector from modetest */
    crtc_id = 41;               /* CRTC ID from modetest */
    
    printf("Using hardcoded working configuration:\n");
    printf("  Connector: %d (DSI-1)\n", connector_id);
    printf("  CRTC: %d\n", crtc_id);
    printf("  Mode: %dx%d@%dHz\n", mode.hdisplay, mode.vdisplay, mode.vrefresh);

    /* Create dumb buffer for framebuffer */
    memset(&create_req, 0, sizeof(create_req));
    create_req.width = mode.hdisplay;   /* 480 pixels */
    create_req.height = mode.vdisplay;  /* 800 pixels */
    create_req.bpp = LCD_BPP;           /* 32 bits per pixel */

    printf("Creating buffer: %dx%d@%dbpp\n", 
           create_req.width, create_req.height, create_req.bpp);

    if (ioctl(drm_fd, DRM_IOCTL_MODE_CREATE_DUMB, &create_req) < 0) {
        printf("Error: Cannot create dumb buffer: %s\n", strerror(errno));
        close(drm_fd);
        drm_fd = -1;
        return HAL_LCD_ERROR;
    }

    printf("Created dumb buffer: handle=%u, pitch=%u, size=%llu\n", 
           create_req.handle, create_req.pitch, create_req.size);

    /* Create framebuffer object */
    struct drm_mode_fb_cmd fb_cmd = {0};
    fb_cmd.width = mode.hdisplay;    /* 480 */
    fb_cmd.height = mode.vdisplay;   /* 800 */
    fb_cmd.pitch = create_req.pitch; /* Bytes per line */
    fb_cmd.bpp = LCD_BPP;            /* 32 */
    fb_cmd.depth = 24;               /* Color depth */
    fb_cmd.handle = create_req.handle;

    if (ioctl(drm_fd, DRM_IOCTL_MODE_ADDFB, &fb_cmd) < 0) {
        printf("Error: Cannot create framebuffer object: %s\n", strerror(errno));
        struct drm_mode_destroy_dumb destroy_req = { .handle = create_req.handle };
        ioctl(drm_fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy_req);
        close(drm_fd);
        drm_fd = -1;
        return HAL_LCD_ERROR;
    }

    fb_id = fb_cmd.fb_id;
    printf("Created framebuffer: ID=%u\n", fb_id);

    /* Map the buffer to user space */
    memset(&map_req, 0, sizeof(map_req));
    map_req.handle = create_req.handle;

    if (ioctl(drm_fd, DRM_IOCTL_MODE_MAP_DUMB, &map_req) < 0) {
        printf("Error: Cannot map dumb buffer: %s\n", strerror(errno));
        ioctl(drm_fd, DRM_IOCTL_MODE_RMFB, &fb_id);
        struct drm_mode_destroy_dumb destroy_req = { .handle = create_req.handle };
        ioctl(drm_fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy_req);
        close(drm_fd);
        drm_fd = -1;
        return HAL_LCD_ERROR;
    }

    printf("Map buffer: offset=%llu\n", map_req.offset);

    /* Memory map the framebuffer */
    buffer_size = create_req.size;
    fb_buffer = (uint32_t *)mmap(0, buffer_size, PROT_READ | PROT_WRITE, 
                                MAP_SHARED, drm_fd, map_req.offset);
    
    if (fb_buffer == MAP_FAILED) {
        printf("Error: Cannot mmap buffer: %s\n", strerror(errno));
        ioctl(drm_fd, DRM_IOCTL_MODE_RMFB, &fb_id);
        struct drm_mode_destroy_dumb destroy_req = { .handle = create_req.handle };
        ioctl(drm_fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy_req);
        close(drm_fd);
        drm_fd = -1;
        return HAL_LCD_ERROR;
    }

    /* Set the display mode and connect framebuffer to display */
    struct drm_mode_crtc crtc = {0};
    crtc.crtc_id = crtc_id;                          /* CRTC 41 */
    crtc.fb_id = fb_id;                              /* Our framebuffer */
    crtc.set_connectors_ptr = (uint64_t)&connector_id; /* DSI connector 34 */
    crtc.count_connectors = 1;                       /* Only one connector */
    crtc.mode = mode;                                /* Our display mode */
    crtc.mode_valid = 1;                             /* Mode is valid */

    if (ioctl(drm_fd, DRM_IOCTL_MODE_SETCRTC, &crtc) < 0) {
        printf("Warning: Cannot set CRTC mode: %s\n", strerror(errno));
        printf("Display might not show on screen\n");
    } else {
        printf("Successfully set display mode\n");
    }

    lcd_initialized = true;
    printf("LCD DRM initialized successfully (%dx%d, %d bpp, buffer size: %zu)\n", 
           mode.hdisplay, mode.vdisplay, LCD_BPP, buffer_size);

    /* Clear screen to red to test */
    hal_lcd_clear(LCD_COLOR_BLACK);

    return HAL_LCD_OK;
}

#endif

hal_lcd_status_t hal_lcd_deinit(void)
{
    if (!lcd_initialized) {
        return HAL_LCD_OK;
    }
    
    printf("Deinitializing LCD DRM...\n");
    
    /* Clear screen */
    hal_lcd_clear(LCD_COLOR_BLACK);
    
    /* Unmap buffer */
    if (fb_buffer != NULL && fb_buffer != MAP_FAILED) {
        munmap(fb_buffer, buffer_size);
        fb_buffer = NULL;
    }

    /* Remove framebuffer */
    if (fb_id) {
        ioctl(drm_fd, DRM_IOCTL_MODE_RMFB, &fb_id);
        fb_id = 0;
    }

    /* Destroy dumb buffer */
    struct drm_mode_destroy_dumb destroy_req;
    memset(&destroy_req, 0, sizeof(destroy_req));
    destroy_req.handle = create_req.handle;
    ioctl(drm_fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy_req);

    /* Close DRM device */
    if (drm_fd >= 0) {
        close(drm_fd);
        drm_fd = -1;
    }

    lcd_initialized = false;
    printf("LCD DRM deinitialized\n");
    return HAL_LCD_OK;
}

hal_lcd_status_t hal_lcd_clear(uint32_t color)
{
    if (!lcd_initialized || fb_buffer == NULL) {
        return HAL_LCD_NOT_INITIALIZED;
    }
    
    printf("Clearing screen with color 0x%08X\n", color);
    
    /* Fill entire buffer with color using actual mode dimensions */
    int pixels = mode.hdisplay * mode.vdisplay;
    for (int i = 0; i < pixels; i++) {
        fb_buffer[i] = color;
    }
    
    return HAL_LCD_OK;
}

hal_lcd_status_t hal_lcd_set_pixel(uint16_t x, uint16_t y, uint32_t color)
{
    if (!lcd_initialized || fb_buffer == NULL) {
        return HAL_LCD_NOT_INITIALIZED;
    }
    
    if (x >= LCD_WIDTH || y >= LCD_HEIGHT) {
        return HAL_LCD_INVALID_PARAM;
    }
    
    fb_buffer[y * LCD_WIDTH + x] = color;
    return HAL_LCD_OK;
}

hal_lcd_status_t hal_lcd_draw_rectangle(hal_lcd_rect_t rect, uint32_t color, bool filled)
{
    if (!lcd_initialized || fb_buffer == NULL) {
        return HAL_LCD_NOT_INITIALIZED;
    }
    
    if (rect.x >= LCD_WIDTH || rect.y >= LCD_HEIGHT) {
        return HAL_LCD_INVALID_PARAM;
    }
    
    /* Clamp rectangle to screen bounds */
    uint16_t end_x = (rect.x + rect.width > LCD_WIDTH) ? LCD_WIDTH : rect.x + rect.width;
    uint16_t end_y = (rect.y + rect.height > LCD_HEIGHT) ? LCD_HEIGHT : rect.y + rect.height;
    
    if (filled) {
        /* Fill rectangle */
        for (uint16_t y = rect.y; y < end_y; y++) {
            for (uint16_t x = rect.x; x < end_x; x++) {
                fb_buffer[y * LCD_WIDTH + x] = color;
            }
        }
    } else {
        /* Draw rectangle outline */
        /* Top and bottom lines */
        for (uint16_t x = rect.x; x < end_x; x++) {
            if (rect.y < LCD_HEIGHT) fb_buffer[rect.y * LCD_WIDTH + x] = color;
            if (end_y - 1 < LCD_HEIGHT) fb_buffer[(end_y - 1) * LCD_WIDTH + x] = color;
        }
        /* Left and right lines */
        for (uint16_t y = rect.y; y < end_y; y++) {
            if (rect.x < LCD_WIDTH) fb_buffer[y * LCD_WIDTH + rect.x] = color;
            if (end_x - 1 < LCD_WIDTH) fb_buffer[y * LCD_WIDTH + (end_x - 1)] = color;
        }
    }
    
    return HAL_LCD_OK;
}
