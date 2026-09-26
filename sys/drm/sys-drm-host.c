// @per_os_impl DRM/KMS host — present, input, audio, sys_* display contract.

#include "base/dbg.h"
#include "base/log.h"
#include "base/marena.h"
#include "base/mem.h"
#include "base/str.h"
#include "engine/gfx/gfx-defs.h"
#include "engine/gfx/gfx.h"
#include "lib/color.h"
#include "lib/tex/tex.h"
#include "sys/sys-debug-draw.h"
#include "sys/sys-gamepad.h"
#include "sys/sys-input.h"
#include "sys/sys-keyboard.h"
#include "sys/sys-opts.h"
#include "sys/sys-os.h"
#include "sys/sys-scoreboards.h"
#include "sys/sys.h"
#include "engine/dbg-drw/dbg-drw.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#include <drm.h>
#include <drm_fourcc.h>
#include <drm_mode.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <linux/kd.h>
#include <linux/input.h>
#include <alsa/asoundlib.h>
#pragma GCC diagnostic pop

#if !defined(DRM_PLANE_ROT_DEG)
#define DRM_PLANE_ROT_DEG 270
#endif

enum drm_status {
	DRM_STATUS_OK,
	DRM_STATUS_ERR,
};

// HDMI scanout: RGB565 dumb buffers, atomic FIT plane.

#define DRM_CARD_PATH_MAX     32
#define DRM_CARD_INDEX_MAX    8
#define DRM_FB_COUNT          2
#define DRM_RGB565_BPP        16
#define DRM_SRC_FIXED_SHIFT   16
#define DRM_TTY_PATH          "/dev/tty0"
#define DRM_SCALE_FILT_NAME   "SCALING_FILTER"
#define DRM_SCALE_FILT_NN_VAL 1

struct drm_fb {
	u32 handle;
	u32 id;
	u32 pitch;
	u32 size;
	void *map;
};

enum drm_obj_kind {
	DRM_OBJ_CONN,
	DRM_OBJ_CRTC,
	DRM_OBJ_PLANE,
};

enum drm_prop {
	DRM_PROP_CONN_CRTC,
	DRM_PROP_CRTC_MODE,
	DRM_PROP_CRTC_ACTIVE,
	DRM_PROP_FB,
	DRM_PROP_CRTC,
	DRM_PROP_SRC_X,
	DRM_PROP_SRC_Y,
	DRM_PROP_SRC_W,
	DRM_PROP_SRC_H,
	DRM_PROP_CRTC_X,
	DRM_PROP_CRTC_Y,
	DRM_PROP_CRTC_W,
	DRM_PROP_CRTC_H,
	DRM_PROP_SCALE_FILT,
	DRM_PROP_COUNT,
};

enum drm_prop_need {
	DRM_PROP_OPTIONAL,
	DRM_PROP_REQUIRED,
};

enum drm_modeset_kind {
	DRM_MODESET_FLIP,
	DRM_MODESET_FULL,
};

struct drm_prop_spec {
	enum drm_obj_kind kind;
	enum drm_prop_need need;
	const char *name;
};

struct drm_display {
	i32 fd;
	i32 tty_fd;
	i32 saved_kd;
	u32 conn_id;
	u32 crtc_id;
	u32 crtc_idx;
	u32 plane_id;
	u32 mode_blob;
	drmModeModeInfo mode;
	u32 props[DRM_PROP_COUNT];
	u32 fb_w;
	u32 fb_h;
	i32 dst_x;
	i32 dst_y;
	i32 dst_w;
	i32 dst_h;
	struct drm_fb bufs[DRM_FB_COUNT];
	i32 back;
	b32 mode_set;
};

static const struct drm_prop_spec DRM_PROP_SPECS[DRM_PROP_COUNT] = {
	{DRM_OBJ_CONN, DRM_PROP_REQUIRED, "CRTC_ID"},
	{DRM_OBJ_CRTC, DRM_PROP_REQUIRED, "MODE_ID"},
	{DRM_OBJ_CRTC, DRM_PROP_REQUIRED, "ACTIVE"},
	{DRM_OBJ_PLANE, DRM_PROP_REQUIRED, "FB_ID"},
	{DRM_OBJ_PLANE, DRM_PROP_REQUIRED, "CRTC_ID"},
	{DRM_OBJ_PLANE, DRM_PROP_REQUIRED, "SRC_X"},
	{DRM_OBJ_PLANE, DRM_PROP_REQUIRED, "SRC_Y"},
	{DRM_OBJ_PLANE, DRM_PROP_REQUIRED, "SRC_W"},
	{DRM_OBJ_PLANE, DRM_PROP_REQUIRED, "SRC_H"},
	{DRM_OBJ_PLANE, DRM_PROP_REQUIRED, "CRTC_X"},
	{DRM_OBJ_PLANE, DRM_PROP_REQUIRED, "CRTC_Y"},
	{DRM_OBJ_PLANE, DRM_PROP_REQUIRED, "CRTC_W"},
	{DRM_OBJ_PLANE, DRM_PROP_REQUIRED, "CRTC_H"},
	{DRM_OBJ_PLANE, DRM_PROP_OPTIONAL, DRM_SCALE_FILT_NAME},
};

static struct drm_display DRM_DISP;

// Oriented scanout size. 90/270: game 400x720 becomes 720x400 for HVS.
static void
drm_fb_wh(u32 *w, u32 *h)
{
	*w = (u32)SYS_DISPLAY_W;
	*h = (u32)SYS_DISPLAY_H;
	if(DRM_PLANE_ROT_DEG == 90 || DRM_PLANE_ROT_DEG == 270) {
		*w = (u32)SYS_DISPLAY_H;
		*h = (u32)SYS_DISPLAY_W;
	}
}

static void
drm_dst_fit(struct drm_display *d)
{
	u32 vis_w  = d->fb_w;
	u32 vis_h  = d->fb_h;
	u32 hdmi_w = d->mode.hdisplay;
	u32 hdmi_h = d->mode.vdisplay;
	u32 dst_w  = hdmi_w;
	u32 dst_h  = hdmi_h;

	if(hdmi_w * vis_h > hdmi_h * vis_w) {
		dst_h = hdmi_h;
		dst_w = vis_w * hdmi_h / vis_h;
	} else {
		dst_w = hdmi_w;
		dst_h = vis_h * hdmi_w / vis_w;
	}

	d->dst_w = (i32)dst_w;
	d->dst_h = (i32)dst_h;
	d->dst_x = (i32)((hdmi_w - dst_w) / 2u);
	d->dst_y = (i32)((hdmi_h - dst_h) / 2u);
}

static u32
drm_prop_id(i32 fd, u32 obj_id, u32 obj_type, const char *name)
{
	drmModeObjectProperties *props = drmModeObjectGetProperties(fd, obj_id, obj_type);
	u32 id                         = 0;
	u32 i                          = 0;

	if(props == NULL) {
		return 0;
	}

	for(i = 0; i < props->count_props; i++) {
		drmModePropertyRes *p = drmModeGetProperty(fd, props->props[i]);
		if(p == NULL) {
			continue;
		}
		if(strcmp(p->name, name) == 0) {
			id = p->prop_id;
		}
		drmModeFreeProperty(p);
		if(id != 0) {
			break;
		}
	}

	drmModeFreeObjectProperties(props);
	return id;
}

static b32
drm_card_has_output(i32 fd)
{
	drmModeRes *res = drmModeGetResources(fd);
	b32 ok          = false;
	i32 i           = 0;

	if(res == NULL) {
		return false;
	}

	for(i = 0; i < res->count_connectors; i++) {
		drmModeConnector *conn = drmModeGetConnector(fd, res->connectors[i]);
		if(conn == NULL) {
			continue;
		}
		if(conn->connection == DRM_MODE_CONNECTED && conn->count_modes > 0) {
			ok = true;
		}
		drmModeFreeConnector(conn);
		if(ok) {
			break;
		}
	}

	drmModeFreeResources(res);
	return ok;
}

static i32
drm_open_card(void)
{
	i32 fd = -1;
	i32 i  = 0;
	char path[DRM_CARD_PATH_MAX];

	for(i = 0; i < DRM_CARD_INDEX_MAX; i++) {
		snprintf(path, sizeof(path), "/dev/dri/card%d", i);
		fd = open(path, O_RDWR | O_CLOEXEC);
		if(fd < 0) {
			continue;
		}
		if(drm_card_has_output(fd)) {
			log_info("drm", "using %s", path);
			break;
		}
		close(fd);
		fd = -1;
	}

	return fd;
}

static enum drm_status
drm_pick_connector(struct drm_display *d, drmModeRes *res)
{
	enum drm_status st = DRM_STATUS_ERR;
	i32 i              = 0;

	for(i = 0; i < res->count_connectors; i++) {
		drmModeConnector *conn = drmModeGetConnector(d->fd, res->connectors[i]);
		if(conn == NULL) {
			continue;
		}
		if(conn->connection == DRM_MODE_CONNECTED && conn->count_modes > 0) {
			d->conn_id = conn->connector_id;
			d->mode    = conn->modes[0];
			st         = DRM_STATUS_OK;
		}
		drmModeFreeConnector(conn);
		if(st == DRM_STATUS_OK) {
			break;
		}
	}

	return st;
}

// HDMI already has encoder_id while fbcon owns the CRTC.
static enum drm_status
drm_pick_crtc(struct drm_display *d, drmModeRes *res)
{
	drmModeConnector *conn = drmModeGetConnector(d->fd, d->conn_id);
	drmModeEncoder *enc    = NULL;
	enum drm_status st     = DRM_STATUS_ERR;
	i32 i                  = 0;

	if(conn == NULL || conn->encoder_id == 0) {
		goto done;
	}

	enc = drmModeGetEncoder(d->fd, conn->encoder_id);
	if(enc == NULL || enc->crtc_id == 0) {
		goto done;
	}

	for(i = 0; i < res->count_crtcs; i++) {
		if(res->crtcs[i] == enc->crtc_id) {
			d->crtc_id  = enc->crtc_id;
			d->crtc_idx = (u32)i;
			st          = DRM_STATUS_OK;
			break;
		}
	}

done:
	if(enc != NULL) {
		drmModeFreeEncoder(enc);
	}
	if(conn != NULL) {
		drmModeFreeConnector(conn);
	}
	return st;
}

static b32
drm_plane_has_fmt(drmModePlane *pl, u32 fmt)
{
	u32 i  = 0;
	b32 ok = false;

	for(i = 0; i < pl->count_formats; i++) {
		if(pl->formats[i] == fmt) {
			ok = true;
			break;
		}
	}

	return ok;
}

// vc4 lists primary first; first RGB565 on this CRTC is the scanout plane.
static enum drm_status
drm_pick_plane(struct drm_display *d)
{
	drmModePlaneRes *planes = drmModeGetPlaneResources(d->fd);
	enum drm_status st      = DRM_STATUS_ERR;
	u32 i                   = 0;

	if(planes == NULL) {
		log_error("drm", "no plane resources (need UNIVERSAL_PLANES)");
		goto done;
	}

	for(i = 0; i < planes->count_planes; i++) {
		drmModePlane *pl = drmModeGetPlane(d->fd, planes->planes[i]);

		if(pl == NULL) {
			continue;
		}
		if((pl->possible_crtcs & (1u << d->crtc_idx)) != 0 &&
			drm_plane_has_fmt(pl, DRM_FORMAT_RGB565)) {
			d->plane_id = pl->plane_id;
			st          = DRM_STATUS_OK;
		}
		drmModeFreePlane(pl);
		if(st == DRM_STATUS_OK) {
			break;
		}
	}

	drmModeFreePlaneResources(planes);
done:
	return st;
}

static enum drm_status
drm_load_props(struct drm_display *d)
{
	enum drm_status st = DRM_STATUS_OK;
	u32 i              = 0;

	for(i = 0; i < (u32)DRM_PROP_COUNT; i++) {
		const struct drm_prop_spec *s = &DRM_PROP_SPECS[i];
		u32 obj                       = d->plane_id;
		u32 type                      = DRM_MODE_OBJECT_PLANE;

		if(s->kind == DRM_OBJ_CONN) {
			obj  = d->conn_id;
			type = DRM_MODE_OBJECT_CONNECTOR;
		} else if(s->kind == DRM_OBJ_CRTC) {
			obj  = d->crtc_id;
			type = DRM_MODE_OBJECT_CRTC;
		}

		d->props[i] = drm_prop_id(d->fd, obj, type, s->name);
		if(s->need == DRM_PROP_REQUIRED && d->props[i] == 0) {
			st = DRM_STATUS_ERR;
			break;
		}
	}

	return st;
}

static void
drm_atomic_fill(struct drm_display *d, drmModeAtomicReq *req, u32 fb_id, enum drm_modeset_kind modeset)
{
	u32 src_w = (u32)d->fb_w << DRM_SRC_FIXED_SHIFT;
	u32 src_h = (u32)d->fb_h << DRM_SRC_FIXED_SHIFT;
	u32 *p    = d->props;

	if(modeset == DRM_MODESET_FULL) {
		drmModeAtomicAddProperty(req, d->conn_id, p[DRM_PROP_CONN_CRTC], d->crtc_id);
		drmModeAtomicAddProperty(req, d->crtc_id, p[DRM_PROP_CRTC_MODE], d->mode_blob);
		drmModeAtomicAddProperty(req, d->crtc_id, p[DRM_PROP_CRTC_ACTIVE], 1);
	}

	drmModeAtomicAddProperty(req, d->plane_id, p[DRM_PROP_FB], fb_id);
	drmModeAtomicAddProperty(req, d->plane_id, p[DRM_PROP_CRTC], d->crtc_id);
	drmModeAtomicAddProperty(req, d->plane_id, p[DRM_PROP_SRC_X], 0);
	drmModeAtomicAddProperty(req, d->plane_id, p[DRM_PROP_SRC_Y], 0);
	drmModeAtomicAddProperty(req, d->plane_id, p[DRM_PROP_SRC_W], src_w);
	drmModeAtomicAddProperty(req, d->plane_id, p[DRM_PROP_SRC_H], src_h);
	drmModeAtomicAddProperty(req, d->plane_id, p[DRM_PROP_CRTC_X], (u64)(u32)d->dst_x);
	drmModeAtomicAddProperty(req, d->plane_id, p[DRM_PROP_CRTC_Y], (u64)(u32)d->dst_y);
	drmModeAtomicAddProperty(req, d->plane_id, p[DRM_PROP_CRTC_W], (u64)(u32)d->dst_w);
	drmModeAtomicAddProperty(req, d->plane_id, p[DRM_PROP_CRTC_H], (u64)(u32)d->dst_h);

	// VC4 HVS bilinear-filters plane scale by default; nearest keeps pixels square.
	if(p[DRM_PROP_SCALE_FILT] != 0) {
		drmModeAtomicAddProperty(req, d->plane_id, p[DRM_PROP_SCALE_FILT], DRM_SCALE_FILT_NN_VAL);
	}
}

static void
drm_fb_destroy(struct drm_display *d, struct drm_fb *fb)
{
	if(fb->map != NULL && fb->size != 0) {
		munmap(fb->map, fb->size);
		fb->map = NULL;
	}
	if(fb->id != 0 && d->fd >= 0) {
		drmModeRmFB(d->fd, fb->id);
		fb->id = 0;
	}
	if(fb->handle != 0 && d->fd >= 0) {
		struct drm_mode_destroy_dumb dreq = {.handle = fb->handle};
		ioctl(d->fd, DRM_IOCTL_MODE_DESTROY_DUMB, &dreq);
		fb->handle = 0;
	}
	fb->pitch = 0;
	fb->size  = 0;
}

static enum drm_status
drm_fb_create(struct drm_display *d, struct drm_fb *fb)
{
	struct drm_mode_create_dumb creq = {0};
	struct drm_mode_map_dumb mreq    = {0};
	u32 handles[4]                   = {0};
	u32 pitches[4]                   = {0};
	u32 offsets[4]                   = {0};
	enum drm_status st               = DRM_STATUS_ERR;

	creq.width  = d->fb_w;
	creq.height = d->fb_h;
	creq.bpp    = DRM_RGB565_BPP;
	if(ioctl(d->fd, DRM_IOCTL_MODE_CREATE_DUMB, &creq) != 0) {
		log_error("drm", "CREATE_DUMB: %s", strerror(errno));
		goto done;
	}

	fb->handle = creq.handle;
	fb->pitch  = creq.pitch;
	fb->size   = creq.size;

	handles[0] = fb->handle;
	pitches[0] = fb->pitch;
	if(drmModeAddFB2(d->fd, d->fb_w, d->fb_h, DRM_FORMAT_RGB565, handles, pitches, offsets, &fb->id, 0) != 0) {
		log_error("drm", "AddFB2 RGB565: %s", strerror(errno));
		goto done;
	}

	mreq.handle = fb->handle;
	if(ioctl(d->fd, DRM_IOCTL_MODE_MAP_DUMB, &mreq) != 0) {
		log_error("drm", "MAP_DUMB: %s", strerror(errno));
		goto done;
	}

	fb->map = mmap(NULL, fb->size, PROT_READ | PROT_WRITE, MAP_SHARED, d->fd, (off_t)mreq.offset);
	if(fb->map == MAP_FAILED) {
		fb->map = NULL;
		log_error("drm", "mmap dumb: %s", strerror(errno));
		goto done;
	}

	memset(fb->map, 0, fb->size);
	st = DRM_STATUS_OK;

done:
	if(st != DRM_STATUS_OK) {
		drm_fb_destroy(d, fb);
	}
	return st;
}

// FULL: modeset. FLIP: flags=0 so commit blocks until vblank.
static enum drm_status
drm_commit(struct drm_display *d, u32 fb_id, enum drm_modeset_kind kind)
{
	drmModeAtomicReq *req = drmModeAtomicAlloc();
	enum drm_status st    = DRM_STATUS_ERR;
	u32 flags             = 0;
	i32 rc                = -1;

	if(req == NULL) {
		goto done;
	}

	if(kind == DRM_MODESET_FULL) {
		flags = DRM_MODE_ATOMIC_ALLOW_MODESET;
	}

	drm_atomic_fill(d, req, fb_id, kind);
	rc = drmModeAtomicCommit(d->fd, req, flags, NULL);
	drmModeAtomicFree(req);

	if(rc != 0) {
		log_error("drm", "atomic commit failed: %s", strerror(errno));
		goto done;
	}

	st = DRM_STATUS_OK;

done:
	return st;
}

static void
drm_tty_graphics(struct drm_display *d)
{
	d->tty_fd   = open(DRM_TTY_PATH, O_RDWR | O_CLOEXEC);
	d->saved_kd = -1;
	if(d->tty_fd < 0) {
		log_warn("drm", "open %s: %s", DRM_TTY_PATH, strerror(errno));
		return;
	}
	if(ioctl(d->tty_fd, KDGETMODE, &d->saved_kd) != 0) {
		log_warn("drm", "KDGETMODE: %s", strerror(errno));
		d->saved_kd = -1;
		return;
	}
	if(ioctl(d->tty_fd, KDSETMODE, KD_GRAPHICS) != 0) {
		log_warn("drm", "KDSETMODE: %s", strerror(errno));
	}
}

static void
drm_tty_restore(struct drm_display *d)
{
	if(d->tty_fd < 0) {
		return;
	}
	if(d->saved_kd >= 0) {
		ioctl(d->tty_fd, KDSETMODE, d->saved_kd);
	}
	close(d->tty_fd);
	d->tty_fd = -1;
}

// RGBA8888 (R in the top byte) -> RGB565.
static u16
drm_rgb565(u32 rgba)
{
	u32 r = (rgba >> 24) & 0xFFu;
	u32 g = (rgba >> 16) & 0xFFu;
	u32 b = (rgba >> 8) & 0xFFu;

	return (u16)(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
}

// Packed 1-bit (MSB-first per byte, 52-byte rows) -> oriented RGB565.
//
//   270: dest(dx, dy) <- game(W-1-dy, dx)   400x720 -> 720x400
//     game x→            dest x→
//   y [A B]            y [B D]
//     [C D]              [A C]
static void
drm_blit(struct drm_display *d, const u8 *packed, u16 col0, u16 col1)
{
	struct drm_fb *fb = &d->bufs[d->back];
	u8 *base          = fb->map;
	u32 pitch         = fb->pitch;
	u32 dy            = 0;

	for(dy = 0; dy < d->fb_h; dy++) {
		u16 *row = (u16 *)(base + (usize)dy * (usize)pitch);
		u32 dx   = 0;

		for(dx = 0; dx < d->fb_w; dx++) {
			i32 gx = 0;
			i32 gy = 0;
			u8 byt = 0;
			u8 bit = 0;

#if DRM_PLANE_ROT_DEG == 90
			gx = (i32)dy;
			gy = (i32)SYS_DISPLAY_H - 1 - (i32)dx;
#elif DRM_PLANE_ROT_DEG == 180
			gx = (i32)SYS_DISPLAY_W - 1 - (i32)dx;
			gy = (i32)SYS_DISPLAY_H - 1 - (i32)dy;
#elif DRM_PLANE_ROT_DEG == 270
			gx = (i32)SYS_DISPLAY_W - 1 - (i32)dy;
			gy = (i32)dx;
#else
			gx = (i32)dx;
			gy = (i32)dy;
#endif
			byt     = packed[(usize)gy * (usize)SYS_DISPLAY_WBYTES + (usize)(gx >> 3)];
			bit     = (u8)(byt & (u8)(0x80u >> (gx & 7)));
			row[dx] = bit != 0 ? col1 : col0;
		}
	}
}

static void drm_display_close(void);

static enum drm_status
drm_display_open(void)
{
	struct drm_display *d = &DRM_DISP;
	drmModeRes *res       = NULL;
	enum drm_status st    = DRM_STATUS_ERR;
	i32 i                 = 0;

	mclr_struct(d);
	d->fd     = -1;
	d->tty_fd = -1;

	d->fd = drm_open_card();
	if(d->fd < 0) {
		log_error("drm", "no DRM card with a connected output");
		goto done;
	}

	// Steal the VT from fbcon, then claim DRM master before modeset.
	drm_tty_graphics(d);
	if(drmSetMaster(d->fd) != 0) {
		log_warn("drm", "drmSetMaster: %s", strerror(errno));
	}

	drmSetClientCap(d->fd, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1);
	if(drmSetClientCap(d->fd, DRM_CLIENT_CAP_ATOMIC, 1) != 0) {
		log_error("drm", "atomic KMS not available");
		goto done;
	}

	res = drmModeGetResources(d->fd);
	if(res == NULL) {
		log_error("drm", "drmModeGetResources failed");
		goto done;
	}

	if(drm_pick_connector(d, res) != DRM_STATUS_OK) {
		log_error("drm", "no connected connector");
		goto done;
	}
	if(drm_pick_crtc(d, res) != DRM_STATUS_OK) {
		log_error("drm", "no CRTC for connector");
		goto done;
	}
	if(drm_pick_plane(d) != DRM_STATUS_OK) {
		log_error("drm", "no RGB565 plane on CRTC");
		goto done;
	}

	drm_fb_wh(&d->fb_w, &d->fb_h);
	drm_dst_fit(d);

	for(i = 0; i < DRM_FB_COUNT; i++) {
		if(drm_fb_create(d, &d->bufs[i]) != DRM_STATUS_OK) {
			goto done;
		}
	}

	if(drm_load_props(d) != DRM_STATUS_OK) {
		log_error("drm", "missing atomic plane/crtc properties");
		goto done;
	}
	if(drmModeCreatePropertyBlob(d->fd, &d->mode, sizeof(d->mode), &d->mode_blob) != 0) {
		log_error("drm", "MODE_ID blob failed: %s", strerror(errno));
		goto done;
	}

	if(d->props[DRM_PROP_SCALE_FILT] != 0) {
		log_info("drm", "plane SCALING_FILTER nearest");
	} else {
		log_warn("drm", "no SCALING_FILTER; plane scale may be bilinear");
	}

	log_info(
		"drm",
		"mode %dx%d src %dx%d dst %d,%d %dx%d plane %u RGB565 rot %d",
		d->mode.hdisplay,
		d->mode.vdisplay,
		d->fb_w,
		d->fb_h,
		d->dst_x,
		d->dst_y,
		d->dst_w,
		d->dst_h,
		d->plane_id,
		DRM_PLANE_ROT_DEG);

	st = DRM_STATUS_OK;

done:
	if(res != NULL) {
		drmModeFreeResources(res);
	}
	if(st != DRM_STATUS_OK) {
		drm_display_close();
	}
	return st;
}

static void
drm_display_close(void)
{
	struct drm_display *d = &DRM_DISP;
	i32 i                 = 0;

	for(i = 0; i < DRM_FB_COUNT; i++) {
		drm_fb_destroy(d, &d->bufs[i]);
	}
	if(d->fd >= 0) {
		if(d->mode_blob != 0) {
			drmModeDestroyPropertyBlob(d->fd, d->mode_blob);
			d->mode_blob = 0;
		}
		drm_tty_restore(d);
		close(d->fd);
		d->fd = -1;
	} else {
		drm_tty_restore(d);
	}
}

static enum drm_status
drm_display_present(const u8 *packed, u32 black, u32 white)
{
	struct drm_display *d      = &DRM_DISP;
	enum drm_status st         = DRM_STATUS_ERR;
	enum drm_modeset_kind kind = DRM_MODESET_FLIP;
	struct drm_fb *fb          = NULL;
	u16 col0                   = 0;
	u16 col1                   = 0;

	if(packed == NULL) {
		goto done;
	}

	fb   = &d->bufs[d->back];
	col0 = drm_rgb565(black);
	col1 = drm_rgb565(white);
	drm_blit(d, packed, col0, col1);

	if(!d->mode_set) {
		kind = DRM_MODESET_FULL;
	}

	st = drm_commit(d, fb->id, kind);
	if(st == DRM_STATUS_OK) {
		d->mode_set = true;
		d->back ^= 1;
	}

done:
	return st;
}

// ALSA playback thread into sys_internal_audio.

#define DRM_ALSA_RATE       44100
#define DRM_ALSA_CHANNELS   1
#define DRM_ALSA_PERIOD     256
#define DRM_ALSA_PERIODS    3
#define DRM_ALSA_BUF_CAP    2048
#define DRM_ALSA_DEVICE     "default"
#define DRM_ALSA_PULSE_MSEC "20"
#define DRM_ALSA_VOL_MIN    0.f
#define DRM_ALSA_VOL_MAX    1.f

struct drm_alsa {
	snd_pcm_t *pcm;
	pthread_t thread;
	pthread_mutex_t lock;
	f32 volume;
	b32 running;
	b32 thread_ok;
	b32 inited;
	i32 channels;
	i32 period;
};

static struct drm_alsa DRM_ALSA;

static i32
drm_alsa_cfg_hw(snd_pcm_t *pcm, snd_pcm_hw_params_t *hw, unsigned int ch, unsigned int *rate, snd_pcm_uframes_t *period, snd_pcm_uframes_t *buf)
{
	i32 dir                = 0;
	i32 rc                 = 0;
	snd_pcm_uframes_t per  = DRM_ALSA_PERIOD;
	snd_pcm_uframes_t want = (snd_pcm_uframes_t)DRM_ALSA_PERIOD * DRM_ALSA_PERIODS;
	unsigned int nperiod   = DRM_ALSA_PERIODS;

	snd_pcm_hw_params_any(pcm, hw);
	rc = snd_pcm_hw_params_set_access(pcm, hw, SND_PCM_ACCESS_RW_INTERLEAVED);
	if(rc >= 0) {
		rc = snd_pcm_hw_params_set_format(pcm, hw, SND_PCM_FORMAT_S16_LE);
	}
	if(rc >= 0) {
		rc = snd_pcm_hw_params_set_channels(pcm, hw, ch);
	}
	if(rc >= 0) {
		rc = snd_pcm_hw_params_set_rate_near(pcm, hw, rate, &dir);
	}
	if(rc >= 0) {
		rc = snd_pcm_hw_params_set_period_size_near(pcm, hw, &per, &dir);
	}
	if(rc >= 0) {
		snd_pcm_hw_params_set_periods_near(pcm, hw, &nperiod, &dir);
		snd_pcm_hw_params_set_buffer_size_near(pcm, hw, &want);
		rc = snd_pcm_hw_params(pcm, hw);
	}
	if(rc < 0) {
		snd_pcm_hw_params_any(pcm, hw);
		rc = snd_pcm_hw_params_set_access(pcm, hw, SND_PCM_ACCESS_RW_INTERLEAVED);
		if(rc >= 0) {
			rc = snd_pcm_hw_params_set_format(pcm, hw, SND_PCM_FORMAT_S16_LE);
		}
		if(rc >= 0) {
			rc = snd_pcm_hw_params_set_channels(pcm, hw, ch);
		}
		if(rc >= 0) {
			rc = snd_pcm_hw_params_set_rate_near(pcm, hw, rate, &dir);
		}
		if(rc >= 0) {
			rc = snd_pcm_hw_params_set_period_size_near(pcm, hw, &per, &dir);
		}
		if(rc >= 0) {
			rc = snd_pcm_hw_params(pcm, hw);
		}
	}
	if(rc >= 0) {
		snd_pcm_hw_params_get_period_size(hw, period, &dir);
		snd_pcm_hw_params_get_buffer_size(hw, buf);
	}
	return rc;
}

static i32
drm_alsa_cfg_sw(snd_pcm_t *pcm, snd_pcm_uframes_t period)
{
	snd_pcm_sw_params_t *sw = NULL;
	i32 rc                  = 0;

	snd_pcm_sw_params_alloca(&sw);
	rc = snd_pcm_sw_params_current(pcm, sw);
	if(rc >= 0) {
		rc = snd_pcm_sw_params_set_start_threshold(pcm, sw, period);
	}
	if(rc >= 0) {
		rc = snd_pcm_sw_params_set_avail_min(pcm, sw, period);
	}
	if(rc >= 0) {
		rc = snd_pcm_sw_params(pcm, sw);
	}
	return rc;
}

static void *
drm_alsa_thread(void *arg)
{
	struct drm_alsa *a = arg;
	i16 lbuf[DRM_ALSA_BUF_CAP];
	i16 rbuf[DRM_ALSA_BUF_CAP];
	i16 out[DRM_ALSA_BUF_CAP * 2];

	while(a->running) {
		i32 n      = 0;
		i32 ch     = a->channels;
		i32 period = a->period;
		i32 maxq   = 0;
		i32 usec   = 0;
		i32 rc     = 0;
		f32 vol    = 0.f;
		snd_pcm_sframes_t wrote;
		snd_pcm_sframes_t delay = 0;

		if(period < 1) {
			period = DRM_ALSA_PERIOD;
		}
		if(period > DRM_ALSA_BUF_CAP) {
			period = DRM_ALSA_BUF_CAP;
		}

		maxq = period * DRM_ALSA_PERIODS;
		rc   = snd_pcm_delay(a->pcm, &delay);
		if(rc < 0) {
			snd_pcm_recover(a->pcm, rc, 1);
			continue;
		}
		if(delay > (snd_pcm_sframes_t)maxq) {
			usec = (period * 1000000) / DRM_ALSA_RATE;
			if(usec < 1000) {
				usec = 1000;
			}
			usleep((useconds_t)usec);
			continue;
		}

		mclr_array(lbuf);
		mclr_array(rbuf);

		pthread_mutex_lock(&a->lock);
		vol = a->volume;
		pthread_mutex_unlock(&a->lock);

		sys_internal_audio(lbuf, rbuf, period);

		if(ch == 1) {
			for(n = 0; n < period; n++) {
				out[n] = (i16)((f32)lbuf[n] * vol);
			}
		} else {
			for(n = 0; n < period; n++) {
				out[n * 2]     = (i16)((f32)lbuf[n] * vol);
				out[n * 2 + 1] = (i16)((f32)lbuf[n] * vol);
			}
		}

		wrote = snd_pcm_writei(a->pcm, out, (snd_pcm_uframes_t)period);
		if(wrote < 0) {
			snd_pcm_recover(a->pcm, (i32)wrote, 1);
		}
	}

	return NULL;
}

static enum drm_status
drm_alsa_open(void)
{
	struct drm_alsa *a       = &DRM_ALSA;
	snd_pcm_hw_params_t *hw  = NULL;
	enum drm_status st       = DRM_STATUS_ERR;
	unsigned int rate        = DRM_ALSA_RATE;
	snd_pcm_uframes_t period = DRM_ALSA_PERIOD;
	snd_pcm_uframes_t buf    = (snd_pcm_uframes_t)DRM_ALSA_PERIOD * DRM_ALSA_PERIODS;
	i32 rc                   = 0;

	mclr_struct(a);
	a->volume  = DRM_ALSA_VOL_MAX;
	a->running = false;
	a->period  = DRM_ALSA_PERIOD;
	pthread_mutex_init(&a->lock, NULL);
	a->inited = true;

	setenv("PULSE_LATENCY_MSEC", DRM_ALSA_PULSE_MSEC, 0);

	if(snd_pcm_open(&a->pcm, DRM_ALSA_DEVICE, SND_PCM_STREAM_PLAYBACK, 0) < 0) {
		log_error("drm", "alsa open '%s' failed", DRM_ALSA_DEVICE);
		goto done;
	}

	snd_pcm_hw_params_alloca(&hw);
	rc = drm_alsa_cfg_hw(a->pcm, hw, DRM_ALSA_CHANNELS, &rate, &period, &buf);
	if(rc < 0) {
		rate   = DRM_ALSA_RATE;
		period = DRM_ALSA_PERIOD;
		buf    = (snd_pcm_uframes_t)DRM_ALSA_PERIOD * DRM_ALSA_PERIODS;
		log_warn("drm", "alsa mono hw_params failed, trying stereo");
		rc = drm_alsa_cfg_hw(a->pcm, hw, 2, &rate, &period, &buf);
		if(rc < 0) {
			log_error("drm", "alsa hw_params failed");
			goto done;
		}
		a->channels = 2;
	} else {
		a->channels = DRM_ALSA_CHANNELS;
	}

	if(period < 1) {
		period = DRM_ALSA_PERIOD;
	}
	if(period > DRM_ALSA_BUF_CAP) {
		period = DRM_ALSA_BUF_CAP;
	}
	a->period = (i32)period;

	if(drm_alsa_cfg_sw(a->pcm, period) < 0) {
		log_warn("drm", "alsa sw_params failed");
	}

	if(snd_pcm_prepare(a->pcm) < 0) {
		log_error("drm", "alsa prepare failed");
		goto done;
	}

	a->running = true;
	if(pthread_create(&a->thread, NULL, drm_alsa_thread, a) != 0) {
		a->running = false;
		log_error("drm", "alsa thread failed");
		goto done;
	}
	a->thread_ok = true;
	st           = DRM_STATUS_OK;
	log_info("drm", "alsa %u Hz ch %d period %lu buf %lu", rate, a->channels, (unsigned long)period, (unsigned long)buf);

done:
	return st;
}

static void
drm_alsa_close(void)
{
	struct drm_alsa *a = &DRM_ALSA;

	if(!a->inited) {
		return;
	}

	a->running = false;
	if(a->thread_ok) {
		pthread_join(a->thread, NULL);
		a->thread_ok = false;
	}
	if(a->pcm != NULL) {
		snd_pcm_close(a->pcm);
		a->pcm = NULL;
	}
	pthread_mutex_destroy(&a->lock);
	a->inited = false;
}

static void
drm_alsa_set_vol(f32 vol)
{
	if(!DRM_ALSA.inited) {
		return;
	}
	if(vol < DRM_ALSA_VOL_MIN) {
		vol = DRM_ALSA_VOL_MIN;
	}
	if(vol > DRM_ALSA_VOL_MAX) {
		vol = DRM_ALSA_VOL_MAX;
	}
	pthread_mutex_lock(&DRM_ALSA.lock);
	DRM_ALSA.volume = vol;
	pthread_mutex_unlock(&DRM_ALSA.lock);
}

static f32
drm_alsa_get_vol(void)
{
	f32 vol = DRM_ALSA_VOL_MAX;
	if(!DRM_ALSA.inited) {
		return vol;
	}
	pthread_mutex_lock(&DRM_ALSA.lock);
	vol = DRM_ALSA.volume;
	pthread_mutex_unlock(&DRM_ALSA.lock);
	return vol;
}

static void
drm_alsa_lock(void)
{
	if(DRM_ALSA.inited) {
		pthread_mutex_lock(&DRM_ALSA.lock);
	}
}

static void
drm_alsa_unlock(void)
{
	if(DRM_ALSA.inited) {
		pthread_mutex_unlock(&DRM_ALSA.lock);
	}
}

// Host: evdev, main loop, sys_* contract.

#define DRM_HOST_ORG          "amano"
#define DRM_HOST_NAME         "luna"
#define DRM_HOST_ARENA_SIZE   MMEGABYTE(2)
#define DRM_HOST_SCRATCH_SIZE MKILOBYTE(256)
#define DRM_IDLE_WAIT_NS      1000000L
#define DRM_EVDEV_MAX         32
#define DRM_EVDEV_DIR         "/dev/input"
#define DRM_EVDEV_PATH_MAX    (sizeof(DRM_EVDEV_DIR) + NAME_MAX + 1)
#define DRM_MOUSE_BTN_LEFT    (1 << 0)
#define DRM_MOUSE_BTN_RIGHT   (1 << 1)
#define DRM_MOUSE_BTN_MID     (1 << 2)

#define drm_bit_word(bit)     ((bit) / (8 * sizeof(unsigned long)))
#define drm_bit_mask(bit)     (1UL << ((bit) % (8 * sizeof(unsigned long))))
#define drm_bit_set(bit, arr) (((arr)[drm_bit_word(bit)] & drm_bit_mask(bit)) != 0)

struct drm_host {
	struct marena arena;
	struct alloc alloc;
	struct marena scratch_arena;
	struct alloc scratch;
	struct gfx_ctx frame_ctx;
	struct gfx_ctx dbg_ctx;
	struct sys_opts opts;
	volatile sig_atomic_t running;
	i32 evdev_fds[DRM_EVDEV_MAX];
	i32 evdev_fd_count;
	i32 mouse_btns;
	f32 mouse_x;
	f32 mouse_y;
	b32 want_quit;
};

static struct drm_host DRM_HOST;

static void
drm_host_on_signal(int sig)
{
	DRM_HOST.running = 0;
}

// CPU unpack+rotate into RGB565; DRM plane FIT-scales to HDMI.
static void
drm_host_present(void)
{
	drm_display_present(
		(const u8 *)DRM_HOST.frame_ctx.dst.px,
		DRM_HOST.opts.colors.colors[GFX_COL_BLACK],
		DRM_HOST.opts.colors.colors[GFX_COL_WHITE]);
}

static void
drm_host_idle(void)
{
	struct timespec ts = {
		.tv_sec  = 0,
		.tv_nsec = DRM_IDLE_WAIT_NS,
	};
	nanosleep(&ts, NULL);
}

static i32
drm_host_key_to_sys(u16 code)
{
	i32 k = 0;

	switch(code) {
	case KEY_A: k = 'A'; break;
	case KEY_B: k = 'B'; break;
	case KEY_C: k = 'C'; break;
	case KEY_D: k = 'D'; break;
	case KEY_E: k = 'E'; break;
	case KEY_F: k = 'F'; break;
	case KEY_G: k = 'G'; break;
	case KEY_H: k = 'H'; break;
	case KEY_I: k = 'I'; break;
	case KEY_J: k = 'J'; break;
	case KEY_K: k = 'K'; break;
	case KEY_L: k = 'L'; break;
	case KEY_M: k = 'M'; break;
	case KEY_N: k = 'N'; break;
	case KEY_O: k = 'O'; break;
	case KEY_P: k = 'P'; break;
	case KEY_Q: k = 'Q'; break;
	case KEY_R: k = 'R'; break;
	case KEY_S: k = 'S'; break;
	case KEY_T: k = 'T'; break;
	case KEY_U: k = 'U'; break;
	case KEY_V: k = 'V'; break;
	case KEY_W: k = 'W'; break;
	case KEY_X: k = 'X'; break;
	case KEY_Y: k = 'Y'; break;
	case KEY_Z: k = 'Z'; break;
	case KEY_0: k = '0'; break;
	case KEY_1: k = '1'; break;
	case KEY_2: k = '2'; break;
	case KEY_3: k = '3'; break;
	case KEY_4: k = '4'; break;
	case KEY_5: k = '5'; break;
	case KEY_6: k = '6'; break;
	case KEY_7: k = '7'; break;
	case KEY_8: k = '8'; break;
	case KEY_9: k = '9'; break;
	case KEY_SPACE: k = ' '; break;
	case KEY_COMMA: k = ','; break;
	case KEY_DOT: k = '.'; break;
	case KEY_UP: k = SYS_OS_KEY_UP; break;
	case KEY_DOWN: k = SYS_OS_KEY_DOWN; break;
	case KEY_LEFT: k = SYS_OS_KEY_LEFT; break;
	case KEY_RIGHT: k = SYS_OS_KEY_RIGHT; break;
	default: break;
	}

	return k;
}

static b32
drm_host_evdev_has_key(i32 fd)
{
	unsigned long evbit[drm_bit_word(EV_MAX) + 1];
	b32 ok = false;

	mclr_array(evbit);
	if(ioctl(fd, EVIOCGBIT(0, sizeof(evbit)), evbit) < 0) {
		goto done;
	}
	ok = drm_bit_set(EV_KEY, evbit);

done:
	return ok;
}

static b32
drm_host_evdev_is_mouse(i32 fd)
{
	unsigned long evbit[drm_bit_word(EV_MAX) + 1];
	b32 ok = false;

	mclr_array(evbit);
	if(ioctl(fd, EVIOCGBIT(0, sizeof(evbit)), evbit) < 0) {
		goto done;
	}
	ok = drm_bit_set(EV_REL, evbit);

done:
	return ok;
}

static void
drm_host_evdev_add_fd(i32 fd)
{
	if(DRM_HOST.evdev_fd_count >= DRM_EVDEV_MAX) {
		close(fd);
	} else {
		fcntl(fd, F_SETFL, O_NONBLOCK);
		DRM_HOST.evdev_fds[DRM_HOST.evdev_fd_count] = fd;
		DRM_HOST.evdev_fd_count++;
	}
}

static enum drm_status
drm_host_evdev_open(void)
{
	DIR *dir           = opendir(DRM_EVDEV_DIR);
	enum drm_status st = DRM_STATUS_OK;
	struct dirent *ent = NULL;

	DRM_HOST.evdev_fd_count = 0;
	DRM_HOST.mouse_btns     = 0;
	DRM_HOST.mouse_x        = (f32)(SYS_DISPLAY_W / 2);
	DRM_HOST.mouse_y        = (f32)(SYS_DISPLAY_H / 2);
	DRM_HOST.want_quit      = false;

	if(dir == NULL) {
		log_warn("drm", "cannot open %s: %s", DRM_EVDEV_DIR, strerror(errno));
		goto done;
	}

	while((ent = readdir(dir)) != NULL) {
		char path[DRM_EVDEV_PATH_MAX];
		i32 fd = -1;
		if(strncmp(ent->d_name, "event", 5) != 0) {
			continue;
		}
		snprintf(path, sizeof(path), "%s/%s", DRM_EVDEV_DIR, ent->d_name);
		fd = open(path, O_RDONLY | O_CLOEXEC);
		if(fd < 0) {
			continue;
		}
		if(drm_host_evdev_has_key(fd) || drm_host_evdev_is_mouse(fd)) {
			drm_host_evdev_add_fd(fd);
		} else {
			close(fd);
		}
	}

	closedir(dir);
	log_info("drm", "evdev devices %d", DRM_HOST.evdev_fd_count);

done:
	return st;
}

static b32
drm_host_is_menu(u16 code)
{
	b32 ok = false;

	switch(code) {
	case KEY_ESC:
	case KEY_ENTER:
	case KEY_MENU:
	case KEY_BACK:
	case KEY_HOMEPAGE:
	case KEY_SELECT:
	case KEY_EXIT:
	case BTN_START:
	case BTN_SELECT:
	case BTN_MODE:
		ok = true;
		break;
	default:
		break;
	}

	return ok;
}

static void
drm_host_on_key(u16 code, b32 down)
{
	i32 sys = drm_host_key_to_sys(code);

	if(drm_host_is_menu(code)) {
		sys_os_keyboard_set('P', down);
		if(down) {
			log_info("drm", "menu key %u -> P", (unsigned)code);
		}
	}

	if(sys != 0) {
		sys_os_keyboard_set(sys, down);
	}
}

static void
drm_host_on_rel(u16 code, i32 value)
{
	if(code == REL_X) {
		DRM_HOST.mouse_x += (f32)value;
	} else if(code == REL_Y) {
		DRM_HOST.mouse_y += (f32)value;
	}

	if(DRM_HOST.mouse_x < 0.f) {
		DRM_HOST.mouse_x = 0.f;
	}
	if(DRM_HOST.mouse_y < 0.f) {
		DRM_HOST.mouse_y = 0.f;
	}
	if(DRM_HOST.mouse_x > (f32)(SYS_DISPLAY_W - 1)) {
		DRM_HOST.mouse_x = (f32)(SYS_DISPLAY_W - 1);
	}
	if(DRM_HOST.mouse_y > (f32)(SYS_DISPLAY_H - 1)) {
		DRM_HOST.mouse_y = (f32)(SYS_DISPLAY_H - 1);
	}
}

static void
drm_host_on_btn(u16 code, b32 down)
{
	i32 bit = 0;

	switch(code) {
	case BTN_LEFT: bit = DRM_MOUSE_BTN_LEFT; break;
	case BTN_RIGHT: bit = DRM_MOUSE_BTN_RIGHT; break;
	case BTN_MIDDLE: bit = DRM_MOUSE_BTN_MID; break;
	default: break;
	}

	if(bit != 0) {
		if(down) {
			DRM_HOST.mouse_btns |= bit;
		} else {
			DRM_HOST.mouse_btns &= ~bit;
		}
	}
}

static void
drm_host_evdev_poll(void)
{
	i32 i = 0;

	for(i = 0; i < DRM_HOST.evdev_fd_count; i++) {
		struct input_event ev;
		while(read(DRM_HOST.evdev_fds[i], &ev, sizeof(ev)) == (ssize_t)sizeof(ev)) {
			b32 down = ev.value != 0;
			if(ev.type == EV_KEY) {
				drm_host_on_key(ev.code, down);
				drm_host_on_btn(ev.code, down);
			} else if(ev.type == EV_REL) {
				drm_host_on_rel(ev.code, ev.value);
			}
		}
	}
}

int
main(int argc, char **argv)
{
	enum drm_status st = DRM_STATUS_OK;
	i32 code           = 1;
	void *mem          = NULL;
	b32 display_ok     = false;
	b32 app_inited     = false;

	sys_os_init();
	mclr_struct(&DRM_HOST);
	DRM_HOST.running = 1;

	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);
	signal(SIGINT, drm_host_on_signal);
	signal(SIGTERM, drm_host_on_signal);

	mem = sys_alloc(NULL, DRM_HOST_ARENA_SIZE, MEM_ALIGN_DEFAULT);
	dbg_check(mem, "drm", "host arena");
	marena_init(&DRM_HOST.arena, mem, DRM_HOST_ARENA_SIZE);
	DRM_HOST.alloc = marena_allocator(&DRM_HOST.arena);

	mem = sys_alloc(NULL, DRM_HOST_SCRATCH_SIZE, MEM_ALIGN_DEFAULT);
	dbg_check(mem, "drm", "host scratch");
	marena_init(&DRM_HOST.scratch_arena, mem, DRM_HOST_SCRATCH_SIZE);
	DRM_HOST.scratch = marena_allocator(&DRM_HOST.scratch_arena);

	DRM_HOST.opts = sys_opts_load(
		DRM_HOST.alloc,
		DRM_HOST.scratch,
		str8_lit(DRM_HOST_ORG),
		str8_lit(DRM_HOST_NAME));

	{
		struct tex tex     = tex_create(DRM_HOST.alloc, SYS_DISPLAY_W, SYS_DISPLAY_H, TEX_FMT_1B_OPAQUE);
		DRM_HOST.frame_ctx = gfx_ctx_default(tex);
		dbg_check(tex.px, "drm", "1-bit framebuffer");
	}

	{
		struct tex tex   = tex_create(DRM_HOST.alloc, SYS_DISPLAY_W, SYS_DISPLAY_H, TEX_FMT_1B_OPAQUE);
		DRM_HOST.dbg_ctx = gfx_ctx_default(tex);
		dbg_check(tex.px, "drm", "dbg framebuffer");
	}

	st = drm_display_open();
	if(st != DRM_STATUS_OK) {
		log_error("drm", "display open");
		goto error;
	}
	display_ok = true;

	drm_host_evdev_open();

	if(drm_alsa_open() != DRM_STATUS_OK) {
		log_warn("drm", "audio disabled");
	}

	sys_internal_init();
	app_inited = true;

	while(DRM_HOST.running && !DRM_HOST.want_quit) {
		i32 drew = 0;
		drm_host_evdev_poll();
		sys_os_gamepad_poll();
		drew = sys_internal_update();
		if(drew) {
			drm_host_present();
			dbg_drw_clr();
		} else {
			drm_host_idle();
		}
	}

	code = 0;

error:
	if(app_inited) {
		sys_internal_close();
	}
	drm_alsa_close();
	/* BlueZ HID close(evdev) never returns. Kernel reaps fds. */
	if(display_ok) {
		drm_display_close();
	}
	if(DRM_HOST.scratch_arena.buf != NULL) {
		sys_free(DRM_HOST.scratch_arena.buf);
		DRM_HOST.scratch_arena.buf = NULL;
	}
	if(DRM_HOST.arena.buf != NULL) {
		sys_free(DRM_HOST.arena.buf);
		DRM_HOST.arena.buf = NULL;
	}
	return code;
}

int
sys_inp(void)
{
	i32 b = sys_os_keyboard_buttons();

	if(DRM_HOST.mouse_btns & DRM_MOUSE_BTN_LEFT) {
		b |= SYS_INP_MOUSE_LEFT;
	}
	if(DRM_HOST.mouse_btns & DRM_MOUSE_BTN_RIGHT) {
		b |= SYS_INP_MOUSE_RIGHT;
	}
	if(DRM_HOST.mouse_btns & DRM_MOUSE_BTN_MID) {
		b |= SYS_INP_MOUSE_MIDDLE;
	}

	b |= sys_os_gamepad_buttons();

	return b;
}

int
sys_key(int k)
{
	int res = sys_os_keyboard_get(k);

	if(k == 'P' && sys_os_gamepad_menu()) {
		res = 1;
	}

	return res;
}

void
sys_keys(u8 *dest, usize count)
{
	sys_os_keyboard_keys(dest, count);
	if(sys_os_gamepad_menu() && count > (usize)'P') {
		dest['P'] = 1;
	}
}

f32
sys_crank(void)
{
	return 0.f;
}

int
sys_crank_docked(void)
{
	return 1;
}

f32
sys_mouse_x(void)
{
	return DRM_HOST.mouse_x;
}

f32
sys_mouse_y(void)
{
	return DRM_HOST.mouse_y;
}

void
sys_1bit_invert(b32 i)
{
}

v4
sys_color_v4_get(enum gfx_col color)
{
	return color_rgba_from_u32(DRM_HOST.opts.colors.colors[color]);
}

void
sys_color_v4_set(enum gfx_col color, v4 value)
{
	DRM_HOST.opts.colors.colors[color] = color_rgba_to_u32(value);
}

u32
sys_color_u32_get(enum gfx_col color)
{
	return DRM_HOST.opts.colors.colors[color];
}

void
sys_color_u32_set(enum gfx_col color, u32 value)
{
	DRM_HOST.opts.colors.colors[color] = value;
}

void *
sys_dbg_buffer(void)
{
	return DRM_HOST.dbg_ctx.dst.px;
}

i32
sys_menu_item_add(const char *title, void (*callback)(void *arg), void *arg)
{
	return 0;
}

i32
sys_menu_checkmark_add(const char *title, int val, void (*callback)(void *arg), void *arg)
{
	return 0;
}

i32
sys_menu_options_add(const char *title, const char **options, int count, void (*callback)(void *arg), void *arg)
{
	return 0;
}

int
sys_menu_value(int id)
{
	return 0;
}

void
sys_menu_item_remove(int id)
{
}

void
sys_menu_clr(void)
{
}

void
sys_set_menu_image(struct tex tex, i32 x_offset)
{
}

void
sys_set_app_name(str8 value)
{
}

void
sys_quit(void)
{
	DRM_HOST.want_quit = true;
}

void
sys_audio_set_volume(f32 vol)
{
	drm_alsa_set_vol(vol);
}

f32
sys_audio_get_volume(void)
{
	return drm_alsa_get_vol();
}

void
sys_audio_lock(void)
{
	drm_alsa_lock();
}

void
sys_audio_unlock(void)
{
	drm_alsa_unlock();
}

int
sys_scores_queries_clear_queue(void)
{
	return 0;
}

int
sys_scores_mutations_clear_queue(void)
{
	return 0;
}

int
sys_score_add(str8 board_id, u32 value, sys_scores_req_callback callback, void *userdata)
{
	return 0;
}

int
sys_scores_get(str8 board_id, sys_scores_req_callback callback, void *userdata, struct alloc alloc)
{
	return 0;
}

int
sys_scores_personal_best_get(str8 board_id, sys_scores_req_callback callback, void *userdata)
{
	return 0;
}
