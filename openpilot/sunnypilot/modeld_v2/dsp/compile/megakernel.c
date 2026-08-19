

#include <hexagon_types.h>
#include <hvx_hexagon_protos.h>
#include "HAP_power.h"
typedef union { struct { void *pv; unsigned int len; } buf; struct { int fd; unsigned int offset; } dma; } remote_arg;
void* HAP_mmap(void *addr, int len, int prot, int flags, int fd, long offset);
int HAP_munmap(void *addr, int len);
unsigned long long HAP_perf_get_time_us(void);
extern void to_d32_asm(const unsigned char* in, int in_width, unsigned char* d32, int next_width_d32, int in_height, int in_depth);
extern void from_d32_asm(const unsigned char* d32, int next_width_d32, unsigned char* out, int in_width, int in_height, int in_depth);
extern void gvconv2dbbb_circ_d32_v65_asm(const unsigned char* input, const signed char* weights, unsigned char* output,
  int in_width_pad, int next_out_width_row, int out_width, int stride_w_h, int indepth, int filt_width, int filt_height,
  int num_out_lines, const int* ptr_wsum, int* ptr_max, const unsigned int* recip_level, int next_out_width,
  unsigned char* circ_buffer, int zshift, int in_offset, const unsigned char* store_ctrl);
extern void gvconv2dbbb_circ_d64_v65_asm(const unsigned char* input, const signed char* weights, unsigned char* output,
  int in_width_pad, int next_out_width_row, int out_width, int stride_w_h, int indepth, int filt_width, int filt_height,
  int num_out_lines, const int* ptr_wsum, int* ptr_max, const unsigned int* recip_level, int next_out_width,
  unsigned char* circ_buffer, int zshift, int in_offset, const unsigned char* store_ctrl);
extern void repstream2_asm(const unsigned char* input, unsigned char* output, int width, int depth, int fill_height,
  int rpad_lpad, int stride_w, const unsigned char* circ_base, int buf_height, int in_offset, int num_accs);
static const unsigned char copy3to4_cntrl[128] __attribute__((aligned(128))) = {
  0x00,0x00,0x00,0x00,0x01,0x03,0x01,0x06,0x02,0x02,0x06,0x04,0x03,0x05,0x0D,0x0C,
  0x04,0x04,0x04,0x00,0x0D,0x0F,0x09,0x0A,0x06,0x02,0x0A,0x08,0x1B,0x19,0x19,0x18,
  0x08,0x08,0x08,0x08,0x09,0x0B,0x01,0x06,0x1A,0x1A,0x1E,0x1C,0x13,0x15,0x15,0x14,
  0x0C,0x0C,0x04,0x00,0x15,0x17,0x11,0x12,0x36,0x32,0x32,0x30,0x33,0x31,0x31,0x30,
  0x10,0x10,0x10,0x10,0x11,0x13,0x11,0x16,0x12,0x12,0x16,0x14,0x03,0x05,0x0D,0x0C,
  0x34,0x34,0x34,0x30,0x3D,0x3F,0x39,0x3A,0x26,0x22,0x2A,0x28,0x2B,0x29,0x29,0x28,
  0x18,0x18,0x18,0x18,0x09,0x0B,0x01,0x06,0x2A,0x2A,0x2E,0x2C,0x23,0x25,0x25,0x24,
  0x6C,0x6C,0x64,0x60,0x65,0x67,0x61,0x62,0x66,0x62,0x62,0x60,0x63,0x61,0x61,0x60,
};
static const unsigned char store_cntrl[384] __attribute__((aligned(128))) = {
  0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,
  0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,
  0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,
  0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,
  0x7,0x7,0x7,0x7,0x7,0x7,0x7,0x7,0x7,0x7,0x7,0x7,0x7,0x7,0x7,0x7,
  0x7,0x7,0x7,0x7,0x7,0x7,0x7,0x7,0x7,0x7,0x7,0x7,0x7,0x7,0x7,0x7,
  0xf,0xf,0xf,0xf,0xf,0xf,0xf,0xf,0xf,0xf,0xf,0xf,0xf,0xf,0xf,0xf,
  0xf,0xf,0xf,0xf,0xf,0xf,0xf,0xf,0xf,0xf,0xf,0xf,0xf,0xf,0xf,0xf,
  0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,
  0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,
  0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,
  0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,
  0x6,0x6,0x6,0x6,0x6,0x6,0x6,0x6,0x6,0x6,0x6,0x6,0x6,0x6,0x6,0x6,
  0x6,0x6,0x6,0x6,0x6,0x6,0x6,0x6,0x6,0x6,0x6,0x6,0x6,0x6,0x6,0x6,
  0xc,0xc,0xc,0xc,0xc,0xc,0xc,0xc,0xc,0xc,0xc,0xc,0xc,0xc,0xc,0xc,
  0xc,0xc,0xc,0xc,0xc,0xc,0xc,0xc,0xc,0xc,0xc,0xc,0xc,0xc,0xc,0xc,
  0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,
  0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,
  0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
  0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
  0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
  0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
  0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
  0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
};

static inline void l2f(const void* p, unsigned width, unsigned height){
  unsigned long long ctl=(((unsigned long long)((1u<<16)|width))<<32)|((width<<16)|height);
  asm volatile(" l2fetch(%0,%1) "::"r"(p),"r"(ctl));
}
enum { PH_CV_PACK=216, PH_CV_COMPUTE, PH_CV_UNPACK, PH_CV_REPAD=229,
       PH_GV_US=236, PH_REP_US, PH_ROW_US,
       PH_SE_GAP=240, PH_SE_RQ, PH_SE_FC1, PH_SE_FC2, PH_DW_REPAD, PH_DW_TILE, PH_DW_UNPACK,
       PH_PF_ISSUE, PH_PF_CLOB, PH_PF_LIVE };
static inline int pfa(void){ unsigned u; asm volatile("%0 = usr":"=r"(u)); return (int)((u>>31)&1); }
#define L2FP(pf, p, w, h) do{ if(pf){ (pf)[PH_PF_ISSUE]++; if(pfa()) (pf)[PH_PF_CLOB]++; } l2f((p),(w),(h)); }while(0)
static inline unsigned long long utimer(void){ unsigned long long t; asm volatile("%0 = utimer":"=r"(t)); return t; }
#define PH_NOW (prof ? utimer() : 0)
#define PHASE(slot) for(unsigned long long _t0=PH_NOW, _once=1; _once; _once=0, (void)(prof && (prof[slot]+=utimer()-_t0)))
typedef HVX_Vector HVX_UVec __attribute__((aligned(1)));
typedef unsigned uint_u __attribute__((aligned(1)));
static inline void d32_copy(unsigned char* d, const unsigned char* s, long nbytes){
  HVX_UVec* dp=(HVX_UVec*)d; const HVX_UVec* sp=(const HVX_UVec*)s; long nv=nbytes>>7;
  for(long i=0;i<nv;i++) dp[i]=sp[i];
  for(long b=nv<<7;b<nbytes;b++) d[b]=s[b]; }
static inline void d32_copy_a(unsigned char* d, const unsigned char* s, long nbytes){
  HVX_Vector* dp=(HVX_Vector*)d; const HVX_Vector* sp=(const HVX_Vector*)s; long nv=nbytes>>7;
  for(long i=0;i<nv;i++) dp[i]=sp[i]; }
static inline void d32_zero(unsigned char* d, long nbytes){
  HVX_UVec* dp=(HVX_UVec*)d; HVX_Vector z=Q6_V_vzero(); long nv=nbytes>>7;
  for(long i=0;i<nv;i++) dp[i]=z;
  for(long b=nv<<7;b<nbytes;b++) d[b]=0; }
static inline void d32_fill(unsigned char* d, int val, long nbytes){
  unsigned r=((unsigned)val&0xff)*0x01010101u; HVX_UVec* dp=(HVX_UVec*)d; HVX_Vector v=Q6_V_vsplat_R(r); long nv=nbytes>>7;
  for(long i=0;i<nv;i++) dp[i]=v;
  for(long b=nv<<7;b<nbytes;b++) d[b]=(unsigned char)val; }
static inline void vstnt(void* p, HVX_Vector v){ asm volatile("vmem(%0+#0):nt = %1"::"r"(p),"v"(v):"memory"); }
#define SCR_TAKE(p, bytes) ({ void* _r__ = (void*)(p); (p) += (((bytes) + 127) & ~127); _r__; })
typedef int HVX_VecW __attribute__((vector_size(128)));
typedef int HVX_VecWU __attribute__((vector_size(128), aligned(1)));
extern int qurt_hvx_lock(int mode);
extern int qurt_hvx_unlock(void);
#ifndef V65_ONE_WT_PF
#define V65_ONE_WT_PF 1
#endif
#ifndef POOL_PRIO
#define POOL_PRIO 100
#endif
typedef struct { char name[16]; unsigned char tcb_partition, affinity; unsigned short priority; unsigned char asid,
  bus_priority; unsigned short timetest_id; unsigned int stack_size; void* stack_addr; char padding[96]; } qurt_thread_attr_t;
extern int qurt_thread_create(unsigned long*, qurt_thread_attr_t*, void(*)(void*), void*);
extern int qurt_thread_join(unsigned long, int*);
extern void qurt_thread_exit(int);
extern void* malloc(unsigned int);
extern void free(void*);
#ifndef CONV_NT
#define CONV_NT 2
#endif
typedef struct { char pad[64]; } qurt_barrier_t;
extern int qurt_barrier_init(qurt_barrier_t*, unsigned int);
extern int qurt_barrier_destroy(qurt_barrier_t*);
extern int qurt_barrier_wait(qurt_barrier_t*);
typedef struct { qurt_barrier_t bstart, bdone; void*(*volatile fn)(void*); void* volatile arg; volatile int stop, up;
  unsigned long th; void* stack; } pool_t;
static void pool_worker(void* a){
  pool_t* p=a;
  qurt_hvx_lock(1);
  for(;;){
    qurt_barrier_wait(&p->bstart);
    if(p->stop) break;
    p->fn(p->arg);
    qurt_barrier_wait(&p->bdone);
  }
  qurt_hvx_unlock();
  qurt_thread_exit(0);
}
static void pool_start(pool_t* p){
  p->stop=0; p->up=0; p->stack=0;
  qurt_barrier_init(&p->bstart,2); qurt_barrier_init(&p->bdone,2);
  qurt_thread_attr_t attr; __builtin_memset(&attr,0,sizeof(attr));
  attr.name[0]='p'; attr.priority=POOL_PRIO; attr.stack_size=(64<<10); attr.stack_addr=malloc(attr.stack_size); p->stack=attr.stack_addr;
  if(attr.stack_addr && qurt_thread_create(&p->th,&attr,pool_worker,p)==0) p->up=1;
  else { qurt_barrier_destroy(&p->bstart); qurt_barrier_destroy(&p->bdone); if(p->stack) free(p->stack); }
}
static void pool_stop(pool_t* p){
  if(!p->up) return;
  p->stop=1;
  qurt_barrier_wait(&p->bstart);
  int st; qurt_thread_join(p->th,&st);
  qurt_barrier_destroy(&p->bstart); qurt_barrier_destroy(&p->bdone); if(p->stack) free(p->stack); p->up=0;
}
static void pool_run2(pool_t* p, void*(*fn)(void*), void* arg0, void* arg1){
  if(!p || !p->up){ fn(arg0); fn(arg1); return; }
  p->fn=fn; p->arg=arg1;
  qurt_barrier_wait(&p->bstart);
  fn(arg0);
  qurt_barrier_wait(&p->bdone);
}
typedef struct { unsigned char *inp,*outp,*circ,*weights; int *biasbuf,*recip;
  int Win,Cigp,in_width_pad,buf_height,in_next_row,stride_wh,rpad_lpad,sw,sh,kh,kw,Wo,Wop,out_next_row,out_chunks,
      w_gcstride,zshift,xzp,oy0,oy1,lock,ochunk_w,pf_wt; long buf_width; unsigned long long* prof; } v65work_t;
static void* v65_tile_worker(void* arg){
  v65work_t* w=arg; unsigned long long* prof=w->prof;
  if(w->lock) qurt_hvx_lock(1);
  int mm[64] __attribute__((aligned(128)));
  for(int i=0;i<32;i++){ mm[i]=-0x7fffffff; mm[32+i]=0x7fffffff; }
  int n_in_rows = w->kh<w->sh ? w->kh : w->sh;
  long inr=w->in_next_row;
  long in_row0 = (long)w->oy0*w->sh;
#ifndef PF_IN_CAP
#define PF_IN_CAP (64*1024)
#endif
  { long span=(long)((w->oy1-w->oy0-1)*w->sh + w->kh)*inr; if(span>PF_IN_CAP) span=PF_IN_CAP;
    unsigned nb=(unsigned)(span/128); if(nb>65535) nb=65535; if(nb) L2FP(prof, w->inp+in_row0*inr, 128, nb); }
  long w_gcs = w->w_gcstride;
#ifndef V65_WT_BUDGET
#define V65_WT_BUDGET (320*1024)
#endif
  int gsize = w->out_chunks;
  if((long)w->out_chunks*w_gcs > V65_WT_BUDGET){ gsize = (int)(V65_WT_BUDGET / w_gcs); if(gsize<1) gsize=1; }
  for(int gc0=0; gc0<w->out_chunks; gc0+=gsize){
    int oc = (gc0+gsize <= w->out_chunks) ? gsize : (w->out_chunks - gc0);
    unsigned char* gwt = w->weights + (long)gc0*w_gcs;
    int* gbias = w->biasbuf + gc0*32; int* grecip = w->recip + gc0*32;
    unsigned char* goutp = w->outp + (long)gc0*w->ochunk_w*32;
    long wtot=(long)oc*w_gcs; unsigned wnb=(unsigned)(wtot/128); if(wnb>65535) wnb=65535;
    if(wnb && w->pf_wt) L2FP(prof, gwt, 128, wnb);
    for(int i=0;i<32;i++){ mm[i]=-0x7fffffff; mm[32+i]=0x7fffffff; }
    repstream2_asm(w->inp+in_row0*inr, w->circ, w->Win, w->Cigp, w->kh, w->rpad_lpad, w->sw, w->circ,
                   w->buf_height, w->xzp, 8);
    int cbuf_row=0;
    for(int oy=w->oy0; oy<w->oy1; oy++){
      unsigned char* cin = w->circ + (long)cbuf_row*w->buf_width;
      unsigned char* orow = goutp + (long)oy*w->out_next_row;
      long newrow = (long)oy*w->sh + w->kh;
      if(oy < w->oy1-1){ unsigned nb=(unsigned)((long)inr*n_in_rows)/128; if(nb) L2FP(prof, w->inp+newrow*inr, 128, nb); }
      if(wnb && w->pf_wt) L2FP(prof, gwt, 128, wnb);
      if(prof && pfa()) prof[PH_PF_LIVE]++;
      PHASE(PH_ROW_US){
      int c=0;
      PHASE(PH_GV_US){
      if(oc & 1){
        gvconv2dbbb_circ_d32_v65_asm(cin, (const signed char*)gwt, orow,
          w->in_width_pad, w->out_next_row, w->Wo, w->stride_wh, w->Cigp, w->kw, w->kh, 1,
          gbias, mm, (const unsigned int*)grecip, w->Wop, w->circ, w->zshift, w->xzp, store_cntrl);
        c=1;
      }
      if(c < oc)
        gvconv2dbbb_circ_d64_v65_asm(cin, (const signed char*)(gwt+(long)c*w_gcs), orow+(long)c*w->ochunk_w*32,
          w->in_width_pad, w->out_next_row, w->Wo, w->stride_wh, w->Cigp, w->kw, w->kh, (oc-c)/2,
          gbias+c*32, mm, (const unsigned int*)(grecip+c*32), w->ochunk_w*32, w->circ, w->zshift, w->xzp, store_cntrl);
      }
      PHASE(PH_REP_US) if(oy < w->oy1-1)
        repstream2_asm(w->inp+newrow*w->in_next_row, cin, w->Win, w->Cigp, n_in_rows, w->rpad_lpad, w->sw,
                       w->circ, w->buf_height, w->xzp, 8);
      }
      cbuf_row += w->sh; if(cbuf_row>=w->buf_height) cbuf_row -= w->buf_height;
    }
  }
  if(w->lock) qurt_hvx_unlock();
  return 0;
}
__attribute__((noinline)) static void conv_repad_d32(unsigned char* dst, const unsigned char* in, int g, int Dg,
    int groups, int H, int W, int Hp, int Win, int ph, int pw, int xzp){
  int iwp=(W+3)&~3; long dstrow=(long)Dg*Win*32, srcrow=(long)groups*Dg*iwp*32;
  d32_fill(dst, xzp, (long)ph*dstrow);
  d32_fill(dst+(long)(ph+H)*dstrow, xzp, (long)(Hp-ph-H)*dstrow);
#ifndef REPAD_PF_ROWS
#define REPAD_PF_ROWS 2
#endif
  { unsigned nb=(unsigned)((srcrow*(REPAD_PF_ROWS<H?REPAD_PF_ROWS:H))/128); if(nb>65535) nb=65535;
    if(nb) l2f(in, 128, nb); }
  for(int h=0;h<H;h++){ unsigned char* row=dst+(long)(ph+h)*dstrow; const unsigned char* srow=in+(long)h*srcrow;
    if(h+REPAD_PF_ROWS<H){ unsigned nb=(unsigned)(srcrow/128); if(nb) l2f(srow+(long)REPAD_PF_ROWS*srcrow, 128, nb); }
    for(int d=0;d<Dg;d++){ unsigned char* ds=row+(long)d*Win*32;
      d32_fill(ds, xzp, (long)pw*32);
      if(!((pw*32)&127) && !((W*32)&127))
           d32_copy_a(ds+(long)pw*32, srow+(long)(g*Dg+d)*iwp*32, (long)W*32);
      else d32_copy(ds+(long)pw*32, srow+(long)(g*Dg+d)*iwp*32, (long)W*32);
      d32_fill(ds+(long)(pw+W)*32, xzp, (long)(Win-pw-W)*32); } }
}
#ifndef PACK_BLK_KB
#define PACK_BLK_KB 16
#endif
#ifndef UNPACK_BLK_KB
#define UNPACK_BLK_KB 32
#endif
static void pack_op(unsigned char* in, int W, unsigned char* out, int onext, int H, int depth,
                    unsigned long long* prof){
  long inrow = (long)W*depth, outrow = (long)onext*(depth>>5);
  if(inrow<=0 || H<=0 || (depth&31)){ to_d32_asm(in, W, out, onext, H, depth); return; }
  int rows = (int)((PACK_BLK_KB*1024L)/inrow); if(rows<1) rows=1; if(rows>H) rows=H;
  { unsigned nb=(unsigned)((inrow*rows)/128); if(nb>65535) nb=65535; if(nb) L2FP(prof, in, 128, nb); }
  for(int y=0; y<H; y+=rows){
    int n = (y+rows<=H) ? rows : (H-y);
    if(y+rows<H){
      int m = (y+2*rows<=H) ? rows : (H-y-rows);
      unsigned nb=(unsigned)((inrow*m)/128); if(nb>65535) nb=65535;
      if(nb) L2FP(prof, in+(long)(y+rows)*inrow, 128, nb);
    }
    to_d32_asm(in+(long)y*inrow, W, out+(long)y*outrow, onext, n, depth);
  }
}
static void unpack_op(unsigned char* d32, int inext, unsigned char* out, int W, int H, int depth,
                      unsigned long long* prof){
  long inrow = (long)inext*(depth>>5), outrow = (long)W*depth;
  if(inrow<=0 || H<=0 || (depth&31)){ from_d32_asm(d32, inext, out, W, H, depth); return; }
  int rows = (int)((UNPACK_BLK_KB*1024L)/inrow); if(rows<1) rows=1; if(rows>H) rows=H;
  { unsigned nb=(unsigned)((inrow*rows)/128); if(nb>65535) nb=65535; if(nb) L2FP(prof, d32, 128, nb); }
  for(int y=0; y<H; y+=rows){
    int n = (y+rows<=H) ? rows : (H-y);
    if(y+rows<H){
      int m = (y+2*rows<=H) ? rows : (H-y-rows);
      unsigned nb=(unsigned)((inrow*m)/128); if(nb>65535) nb=65535;
      if(nb) L2FP(prof, d32+(long)(y+rows)*inrow, 128, nb);
    }
    from_d32_asm(d32+(long)y*inrow, inext, out+(long)y*outrow, W, n, depth);
  }
}

static void conv_op(unsigned char* in,unsigned char* out,unsigned char* d32in,unsigned char* d32out,
   unsigned char* weights,int* biasbuf,int* recip,int* minmax,int* p,unsigned long long* prof,
   unsigned char* arena,pool_t* pool){
  int Cin=p[P_Cin],H=p[P_H],W=p[P_W],Cout=p[P_Cout],kh=p[P_kh],kw=p[P_kw],sh=p[P_sh],sw=p[P_sw];
  int ph=p[P_ph],pw=p[P_pw],groups=p[P_groups];
  int xzp=p[P_xzp],zshift=p[P_zshift],Ho=p[P_Ho],Wo=p[P_Wo],Wop=p[P_Wop],Win=p[P_Win],Hp=p[P_Hp];
  int w_gcstride=p[P_w_gcstride];
  int Cig=Cin/groups, Cog=Cout/groups, Cigp=p[P_Cigp], Cogp=p[P_Cogp];
  int out_chunks=Cogp/32, totchunks=p[P_tot];
  int in_gstride=Hp*(Cigp/32)*Win*32;
  int out_next_row=Wop*32*totchunks, stride_hw=(sh<<16)|sw;
  int in_d32=p[P_in_d32], out_d32=p[P_out_d32];
  unsigned char* g_circ = arena + p[P_circ];
  int b_chunk_w=p[P_bord_Wp], b_base_off=p[P_bord_base];
  int ochunk_w = b_chunk_w ? b_chunk_w : Wop;
  int onr_out  = b_chunk_w ? (totchunks*ochunk_w*32) : out_next_row;
  int repad = in_d32 && (ph>0 || pw>0);
  unsigned char* d32ip = (in_d32 && !repad) ? in : d32in;
  unsigned char* d32op = out_d32 ? out : d32out;
  for(int g=0; g<groups; g++){
    PHASE(PH_CV_PACK){
      if(!in_d32) to_d32_asm(in, W, d32ip, Win*32, H, Cigp);
      else if(repad) PHASE(PH_CV_REPAD)
        conv_repad_d32(d32ip+(long)g*in_gstride, in, g, Cigp/32, groups, H, W, Hp, Win, ph, pw, xzp);
    }
    PHASE(PH_CV_COMPUTE){
      int in_right_padpad = 8*sw;
      int in_lskip = p[P_in_left_skip];
      int in_width_pad = (Win - in_lskip + 3 + in_right_padpad) & ~3;
      int buf_height = kh>sh ? kh : sh;
      long buf_width = (long)in_width_pad*2*Cigp;
      long slice = (buf_width*buf_height + 127) & ~127L;
      int g0 = g*out_chunks;
      v65work_t base = { d32ip+(long)g*in_gstride, d32op+b_base_off+(long)g0*ochunk_w*32, g_circ,
        weights+(long)g0*w_gcstride, biasbuf+g0*32, recip+g0*32,
        Win, Cigp, in_width_pad, buf_height, (Cigp/32)*Win*32, (sh<<16)|sw, (in_right_padpad<<16)|(in_lskip&0xffff), sw, sh, kh, kw,
        Wo, Wop, onr_out, out_chunks, w_gcstride, zshift, xzp, 0, Ho, 0, ochunk_w, 1, buf_width, prof };
      if(Ho < CONV_NT){ base.oy1 = Ho; v65_tile_worker(&base); }
      else {
        int rp = (Ho + CONV_NT - 1) / CONV_NT;
        v65work_t w0=base, w1=base;
        w0.oy0=0;  w0.oy1=rp;   w0.pf_wt=1;
        w1.oy0=rp; w1.oy1=Ho;   w1.pf_wt=!V65_ONE_WT_PF;
        w1.circ = g_circ + slice;
        pool_run2(pool, v65_tile_worker, &w0, &w1);
      }
    }
  }
  PHASE(PH_CV_UNPACK) if(!out_d32) unpack_op(d32op, Wop*32, out, Wo, Ho, Cout, prof);
}

static float clampf(float v,float lo,float hi){ return v<lo?lo:(v>hi?hi:v); }
static int roundi(float v){ return (int)(v>=0.0f?v+0.5f:v-0.5f); }

static int dot_u8i8(unsigned char* a, signed char* w, int K){
  int nv=K/128, s=0; HVX_Vector acc=Q6_V_vzero();
  HVX_Vector* av=(HVX_Vector*)a; HVX_Vector* wv=(HVX_Vector*)w;
  for(int v=0;v<nv;v++) acc=Q6_Vw_vrmpyacc_VwVubVb(acc, av[v], wv[v]);
  for(int r=64;r>=4;r>>=1) acc=Q6_Vw_vadd_VwVw(acc, Q6_V_vror_VR(acc,r));
  s=Q6_R_vextract_VR(acc,0);
  for(int i=nv*128;i<K;i++) s+=a[i]*w[i];
  return s;
}
static void gap_accum_d32(unsigned char* restrict in, int* restrict acc_chan, int C, int Ho, int Wo, int Wop, int* restrict atab){
  int nchunk=C/32; long rowstride=(long)nchunk*Wop*32; int wg=Wo>>2, wtail=Wo&3;
  for(int i=0;i<nchunk*128;i++) atab[i]=0;
  unsigned pfh=(unsigned)rowstride/128;
  if(pfh) l2f(in,128,pfh);
  for(int h=0; h<Ho; h++){
    if(pfh && h+1<Ho) l2f(in+(long)(h+1)*rowstride,128,pfh);
    for(int ch=0; ch<nchunk; ch++){
      HVX_Vector* iv=(HVX_Vector*)(in + (long)h*rowstride + (long)ch*Wop*32);
      HVX_Vector* ap=(HVX_Vector*)(atab+ch*128);
      HVX_Vector a0=ap[0],a1=ap[1],a2=ap[2],a3=ap[3];
      for(int g=0; g<wg; g++){
        HVX_VectorPair hh=Q6_Wuh_vzxt_Vub(iv[g]);
        HVX_VectorPair l0=Q6_Wuw_vzxt_Vuh(Q6_V_lo_W(hh)), l1=Q6_Wuw_vzxt_Vuh(Q6_V_hi_W(hh));
        a0=Q6_Vw_vadd_VwVw(a0,Q6_V_lo_W(l0)); a1=Q6_Vw_vadd_VwVw(a1,Q6_V_hi_W(l0));
        a2=Q6_Vw_vadd_VwVw(a2,Q6_V_lo_W(l1)); a3=Q6_Vw_vadd_VwVw(a3,Q6_V_hi_W(l1));
      }
      ap[0]=a0; ap[1]=a1; ap[2]=a2; ap[3]=a3;
    }
  }
  for(int ch=0; ch<nchunk; ch++){ int* a128=atab+ch*128;
    for(int c=0;c<32;c++){
      int s=0;
      for(int slot=0;slot<4;slot++){ int b=slot*32+c, rr=b&3, gg=(rr==1?2:(rr==2?1:rr)); s+=a128[(gg<<5)+(b>>2)]; }
      acc_chan[ch*32+c]=s;
    }
    if(wtail){ unsigned char* p0=in + (long)ch*Wop*32;
      for(int c=0;c<32;c++){ int s=acc_chan[ch*32+c];
        for(int h=0;h<Ho;h++){ unsigned char* p=p0+(long)h*rowstride; for(int w=wg*4;w<Wo;w++) s+=p[w*32+c]; }
        acc_chan[ch*32+c]=s; } }
  }
}
typedef struct { const unsigned char* src; const HVX_Vector* zpv; const HVX_Vector* fv; } ewstep_t;
#ifndef NT_LOADS
#define NT_LOADS 0
#endif
static inline HVX_Vector vldnt(const void* p){ HVX_Vector v; asm volatile("%0 = vmem(%1+#0):nt":"=v"(v):"r"(p)); return v; }

__attribute__((always_inline)) static inline void ew_mul(const ewstep_t* s, long b, int ua, HVX_VectorPair* ml, HVX_VectorPair* mh){
  HVX_Vector zp=s->zpv[0], v= ua ? ((const HVX_UVec*)s->src)[b]
      : (NT_LOADS ? vldnt((const HVX_Vector*)s->src + b) : ((const HVX_Vector*)s->src)[b]);
  HVX_VectorPair p=Q6_Wuh_vzxt_Vub(v);
  *ml=Q6_Ww_vmpy_VhVh(Q6_Vh_vsub_VhVh(Q6_V_lo_W(p),zp),s->fv[0]);
  *mh=Q6_Ww_vmpy_VhVh(Q6_Vh_vsub_VhVh(Q6_V_hi_W(p),zp),s->fv[1]);
}
__attribute__((always_inline)) static inline HVX_Vector ewise_blk(const ewstep_t* restrict st, int nst, long b,
    HVX_Vector vzo, int S, int clampq, HVX_Vector vqm, int ua){
  HVX_VectorPair ml,mh; ew_mul(st,b,ua,&ml,&mh);
  HVX_Vector alo=Q6_V_lo_W(ml), ahi=Q6_V_hi_W(ml), blo=Q6_V_lo_W(mh), bhi=Q6_V_hi_W(mh);
  for(int i=1;i<nst;i++){ ew_mul(st+i,b,ua,&ml,&mh);
    alo=Q6_Vw_vadd_VwVw(alo,Q6_V_lo_W(ml)); ahi=Q6_Vw_vadd_VwVw(ahi,Q6_V_hi_W(ml));
    blo=Q6_Vw_vadd_VwVw(blo,Q6_V_lo_W(mh)); bhi=Q6_Vw_vadd_VwVw(bhi,Q6_V_hi_W(mh)); }
  HVX_Vector rlo=Q6_Vh_vasr_VwVwR_rnd_sat(ahi,alo,S), rhi=Q6_Vh_vasr_VwVwR_rnd_sat(bhi,blo,S);
  rlo=Q6_Vh_vadd_VhVh(rlo,vzo); rhi=Q6_Vh_vadd_VhVh(rhi,vzo);
  if(clampq){ rlo=Q6_Vh_vmin_VhVh(rlo,vqm); rhi=Q6_Vh_vmin_VhVh(rhi,vqm); }
  return Q6_Vub_vasr_VhVhR_rnd_sat(rhi,rlo,0);
}
static void setail_d32(unsigned char* restrict out, unsigned char* restrict conv, unsigned char* restrict res, float* gate, float* blob,
   int C,int Ho,int Wo,int Wop,int zc,int zr,int zo,int relu,int hasres,int has_gate,int out_d32, unsigned char* scratch){
  float Asc=blob[0], Asr=blob[1]; float mx=Asc>Asr?Asc:Asr; if(mx<1.0f) mx=1.0f;
  int S=15; while(mx*(float)(1<<S) >= 32760.0f) S--;
  int rf=roundi(Asr*(float)(1<<S));
  HVX_Vector kv[4] __attribute__((aligned(128)));
  kv[0]=Q6_V_vsplat_R(((zc&0xffff)<<16)|(zc&0xffff));
  kv[1]=Q6_V_vsplat_R(((zr&0xffff)<<16)|(zr&0xffff));
  kv[2]=kv[3]=Q6_V_vsplat_R(((rf&0xffff)<<16)|(rf&0xffff));
  HVX_Vector vzo=Q6_V_vsplat_R(((zo&0xffff)<<16)|(zo&0xffff));
  int nchunk=C/32; long rowstride=(long)nchunk*Wop*32; int wg=Wo>>2, wtail=Wo&3;
  short* gqtab = SCR_TAKE(scratch, C*8);
  for(int ch=0; ch<nchunk; ch++) for(int k=0;k<64;k++){
    float ge=has_gate?gate[ch*32+2*(k&15)]:1.0f, go=has_gate?gate[ch*32+2*(k&15)+1]:1.0f;
    gqtab[ch*128+k]=(short)roundi(Asc*ge*(float)(1<<S)); gqtab[ch*128+64+k]=(short)roundi(Asc*go*(float)(1<<S));
  }
  unsigned pfh=(unsigned)rowstride/128;
  if(pfh){ l2f(conv,128,pfh); if(hasres) l2f(res,128,pfh); }
  for(int h=0; h<Ho; h++){
    if(pfh && h+1<Ho){ l2f(conv+(long)(h+1)*rowstride,128,pfh); if(hasres) l2f(res+(long)(h+1)*rowstride,128,pfh); }
    for(int ch=0; ch<nchunk; ch++){
      unsigned char* pc=conv+(long)h*rowstride+(long)ch*Wop*32;
      unsigned char* pr=hasres?res+(long)h*rowstride+(long)ch*Wop*32:pc;
      unsigned char* pod=out+(long)h*rowstride+(long)ch*Wop*32;
      unsigned char* pon=out+(long)h*Wo*C+ch*32;
      HVX_Vector* o=(HVX_Vector*)pod;
      ewstep_t st[2]={{pc,kv+0,(const HVX_Vector*)(gqtab+ch*128)},{pr,kv+1,kv+2}};
      for(int g=0; g<wg; g++){
        HVX_Vector rv=ewise_blk(st,hasres?2:1,g,vzo,S,0,vzo,0);
        if(out_d32) o[g]=rv;
        else { unsigned char tb[128] __attribute__((aligned(128))); *(HVX_Vector*)tb=rv;
          for(int p=0;p<4;p++) __builtin_memcpy(pon+(long)(g*4+p)*C, tb+p*32, 32); }
      }
      for(int wp=wg*4; wp<Wo; wp++) for(int l=0;l<32;l++){
        int c=ch*32+l; float v=Asc*(has_gate?gate[c]:1.0f)*((float)pc[wp*32+l]-zc);
        if(hasres) v+=Asr*((float)pr[wp*32+l]-zr);
        if(relu&&v<0.0f) v=0; int q=roundi(v)+zo; if(q<0)q=0; if(q>255)q=255;
        (out_d32?pod:pon)[out_d32? wp*32+l : (long)wp*C+l]=(unsigned char)q;
      }
    }
  }
}
static void se_gate(float* gate, unsigned char* expand, signed char* fc1w, signed char* fc2w,
   float* blob, float* lut, int Cexp,int Csq, int Ho,int Wo,int Wop, unsigned char* scratch,
   unsigned long long* prof){
  float g0=blob[0],g1=blob[1],inv_srm=blob[2],z_rm=blob[3];
  float* M1=blob+4; float* b1q=M1+Csq; float* W1=b1q+Csq;
  int Cexpp=(Cexp+127)&~127;
  int* mAi=(int*)(W1+Csq); int* mBi=mAi+Cexpp; int Ssig=*(int*)(mBi+Cexpp);
  l2f(lut, 128, 128);
  { unsigned h1=(unsigned)(Csq*Cexpp)/128; if(h1){ l2f(fc1w,128,h1); l2f(fc2w,128,h1); } }
  int* atab = SCR_TAKE(scratch, Cexp / 32 * 128 * 4);
  int* acc = SCR_TAKE(scratch, Cexpp * 4);
  unsigned char* xqu = SCR_TAKE(scratch, Cexp);
  float* fc1u = SCR_TAKE(scratch, Csq * 4);
  PHASE(PH_SE_GAP) gap_accum_d32(expand, acc, Cexp, Ho, Wo, Wop, atab);
  PHASE(PH_SE_RQ){
    const int HW = Ho*Wo;
    int zc = roundi(g1/(g0*(float)HW));
    int zsum = zc*HW;
    enum { S = 21 };
    int Ai = roundi(g0*inv_srm*(float)(1<<S));
    float limf = 2147000000.0f/(float)(Ai > 1 ? Ai : 1);
    int lim = (int)(limf > 2.0e9f ? 2.0e9f : limf);
    HVX_VecW vA = (HVX_VecW)Q6_V_vsplat_R(Ai), vB = (HVX_VecW)Q6_V_vsplat_R(1<<(S-1));
    HVX_Vector vZ = Q6_V_vsplat_R(zsum), vL = Q6_V_vsplat_R(lim), vNL = Q6_V_vsplat_R(-lim);
    HVX_Vector vzr = Q6_V_vsplat_R((int)z_rm), vlo = Q6_V_vzero(), vhi255 = Q6_V_vsplat_R(255);
    HVX_Vector* av = (HVX_Vector*)acc;
    for(int v=0; v<Cexp/32; v++){
      HVX_Vector d = Q6_Vw_vsub_VwVw(av[v], vZ);
      d = Q6_Vw_vmin_VwVw(Q6_Vw_vmax_VwVw(d, vNL), vL);
      HVX_VecW p = (HVX_VecW)d*vA + vB;
      HVX_Vector h = Q6_Vw_vadd_VwVw(Q6_Vw_vasr_VwR((HVX_Vector)p, S), vzr);
      av[v] = Q6_Vw_vmin_VwVw(Q6_Vw_vmax_VwVw(h, vlo), vhi255);
    }
    for(int c=0;c<Cexp;c++) xqu[c] = (unsigned char)acc[c];
  }
  PHASE(PH_SE_FC1) for(int o=0;o<Csq;o++){
    float a=(float)dot_u8i8(xqu, fc1w+(long)o*Cexpp, Cexp) - z_rm*W1[o];
    fc1u[o]=clampf((float)roundi((a+b1q[o])*M1[o]),0,255);
  }
  int* acc2=acc;
  int nbk=Cexpp/128;
  PHASE(PH_SE_FC2){
    HVX_Vector* a2=(HVX_Vector*)acc2;
    for(int v=0;v<nbk*4;v++) a2[v]=Q6_V_vzero();
    for(int o=0;o<Csq;o++){
      HVX_Vector* wv=(HVX_Vector*)(fc2w+(long)o*Cexpp);
      unsigned f=((unsigned)(int)fc1u[o])*0x01010101u;
      for(int blk=0;blk<nbk;blk++){
        HVX_VectorPair wh=Q6_Wh_vsxt_Vb(wv[blk]);
        HVX_VectorPair w0=Q6_Ww_vsxt_Vh(Q6_V_lo_W(wh)), w1=Q6_Ww_vsxt_Vh(Q6_V_hi_W(wh));
        a2[blk*4+0]=Q6_Vw_vmpyiacc_VwVwRub(a2[blk*4+0],Q6_V_lo_W(w0),f);
        a2[blk*4+1]=Q6_Vw_vmpyiacc_VwVwRub(a2[blk*4+1],Q6_V_hi_W(w0),f);
        a2[blk*4+2]=Q6_Vw_vmpyiacc_VwVwRub(a2[blk*4+2],Q6_V_lo_W(w1),f);
        a2[blk*4+3]=Q6_Vw_vmpyiacc_VwVwRub(a2[blk*4+3],Q6_V_hi_W(w1),f);
      }
    }
    HVX_Vector vhi=Q6_V_vsplat_R(4095), vz=Q6_V_vzero();
    HVX_VecWU* mav=(HVX_VecWU*)mAi; HVX_VecWU* mbv=(HVX_VecWU*)mBi;
    for(int v=0;v<nbk*4;v++){
      HVX_VecW p=(HVX_VecW)((HVX_Vector*)acc2)[v] * mav[v] + mbv[v];
      HVX_Vector idxv=Q6_Vw_vmin_VwVw(Q6_Vw_vmax_VwVw(Q6_Vw_vasr_VwR((HVX_Vector)p,Ssig),vz),vhi);
      ((HVX_Vector*)acc2)[v]=idxv;
    }
    for(int c=0;c<Cexp;c++){
      int l=c&127,rr=l&3,gg=(rr==1?2:(rr==2?1:rr)); gate[c]=lut[acc2[(c&~127)+(gg<<5)+(l>>2)]];
    }
  }
}
#define EW_MAXST 4
typedef struct { unsigned char *src; int zp, f; } ewacc_t;
static void ewise_op(unsigned char* restrict out, const ewacc_t* st, int nst, int S, int zo, int qmax, long n){
  HVX_Vector vzo=Q6_V_vsplat_R(((zo&0xffff)<<16)|(zo&0xffff)), vqm=Q6_V_vsplat_R(((qmax&0xffff)<<16)|(qmax&0xffff));
  int clampq=(qmax<255);
  HVX_Vector kv[3*EW_MAXST] __attribute__((aligned(128))); ewstep_t es[EW_MAXST];
  for(int i=0;i<nst;i++){
    kv[3*i]=Q6_V_vsplat_R(((st[i].zp&0xffff)<<16)|(st[i].zp&0xffff));
    kv[3*i+1]=kv[3*i+2]=Q6_V_vsplat_R(((st[i].f&0xffff)<<16)|(st[i].f&0xffff));
    es[i]=(ewstep_t){st[i].src, kv+3*i, kv+3*i+1};
  }
  long nv=n/128; HVX_Vector* ov=(HVX_Vector*)out;
  const long CB=32;
  if(nv) for(int i=0;i<nst;i++) l2f(st[i].src,128,(unsigned)(nv<CB?nv:CB));
  for(long v=0;v<nv;v++){
    if((v%CB)==0 && v+2*CB<nv) for(int i=0;i<nst;i++) l2f(st[i].src+(v+2*CB)*128,128,CB);
    ov[v]=ewise_blk(es,nst,v,vzo,S,clampq,vqm,0);
  }
  for(long i=nv*128;i<n;i++){ long long acc=0;
    for(int k=0;k<nst;k++) acc+=(long long)(st[k].src[i]-st[k].zp)*st[k].f;
    long long q=((acc+(1LL<<(S-1)))>>S)+zo; if(q<0)q=0; if(q>qmax)q=qmax; out[i]=(unsigned char)q; }
}
static inline HVX_Vector requant_vec(HVX_Vector s, HVX_Vector recip, int zsh){
  HVX_Vector t = zsh ? Q6_Vw_vasl_VwR(s, zsh) : s;
  HVX_Vector y = Q6_Vw_vmpye_VwVuh(t, recip);
  y = Q6_Vw_vmpyoacc_VwVwVh_s1_rnd_sat_shift(y, t, recip);
  y = Q6_Vw_vmax_VwVw(y, Q6_V_vzero());
  return Q6_Vw_vmin_VwVw(y, Q6_V_vsplat_R(255));
}
#ifndef WP_PIL
#error "WP_PIL must be -D'd in from lower.py (it also sizes the cp4 arena there)"
#endif
static inline void stem_pil_fill(unsigned char* crow, unsigned char* in, int iy, int H, int W, int Cin, int pad, int xzp){
  HVX_Vector fv = Q6_V_vsplat_R((unsigned)(xzp & 0xff) * 0x01010101u);
  for(int i=0; i<WP_PIL*4/128; i++) ((HVX_Vector*)crow)[i] = fv;
  if(iy>=0 && iy<H){ unsigned char* irow = in + (long)iy*W*Cin;
    HVX_Vector ctrl = *(const HVX_Vector*)copy3to4_cntrl; int c=0;
    for(; c+32<=W; c+=32){ HVX_Vector v=*(HVX_UVec*)(irow+(long)c*3);
      *(HVX_UVec*)(crow+(long)(pad+c)*4) = Q6_V_vdelta_VV(v, ctrl); }
    for(; c<W; c++){ unsigned char* d=crow+(long)(pad+c)*4; d[0]=irow[c*3]; d[1]=irow[c*3+1]; d[2]=irow[c*3+2]; d[3]=0; }
  }
}
static void stem_pil_out_row(unsigned char* out_row, unsigned char* cp4_3, const int* wq, int* bias, int* recip,
    int Wo, int Cout, int k, int zsh){
  for(int ox0=0; ox0<Wo; ox0+=32){
    int npix = Wo-ox0<32 ? Wo-ox0 : 32;
    HVX_Vector iv[9];
    for(int ky=0; ky<k; ky++){ unsigned char* crow = cp4_3 + (long)ky*WP_PIL*4;
      for(int kx=0; kx<k; kx++){ int s0 = 2*ox0 + kx;
        HVX_Vector vlo=*(HVX_UVec*)(crow+(long)s0*4), vhi=*(HVX_UVec*)(crow+(long)(s0+32)*4);
        iv[ky*k+kx] = Q6_V_lo_W(Q6_W_vdeal_VVR(vhi, vlo, -4)); } }
    unsigned char* orow = out_row + (long)ox0*Cout;
    unsigned char g4s[8*128] __attribute__((aligned(128)));
    #define G4(w0_,x_,y_,z_) Q6_V_vor_VV(Q6_V_vor_VV((w0_), Q6_Vw_vasl_VwR((x_),8)), Q6_V_vor_VV(Q6_Vw_vasl_VwR((y_),16), Q6_Vw_vasl_VwR((z_),24)))
    for(int sg=0; sg<4; sg++){ int bb=8*sg;
      HVX_Vector a0=Q6_V_vsplat_R(bias[bb+0]),a1=Q6_V_vsplat_R(bias[bb+1]),a2=Q6_V_vsplat_R(bias[bb+2]),a3=Q6_V_vsplat_R(bias[bb+3]),
                 a4=Q6_V_vsplat_R(bias[bb+4]),a5=Q6_V_vsplat_R(bias[bb+5]),a6=Q6_V_vsplat_R(bias[bb+6]),a7=Q6_V_vsplat_R(bias[bb+7]);
      for(int t=0;t<9;t++){ HVX_Vector x=iv[t]; const int* w=wq+t*32+bb;
        a0=Q6_Vw_vrmpyacc_VwVubRb(a0,x,w[0]); a1=Q6_Vw_vrmpyacc_VwVubRb(a1,x,w[1]);
        a2=Q6_Vw_vrmpyacc_VwVubRb(a2,x,w[2]); a3=Q6_Vw_vrmpyacc_VwVubRb(a3,x,w[3]);
        a4=Q6_Vw_vrmpyacc_VwVubRb(a4,x,w[4]); a5=Q6_Vw_vrmpyacc_VwVubRb(a5,x,w[5]);
        a6=Q6_Vw_vrmpyacc_VwVubRb(a6,x,w[6]); a7=Q6_Vw_vrmpyacc_VwVubRb(a7,x,w[7]); }
      a0=requant_vec(a0,Q6_V_vsplat_R(recip[bb+0]),zsh); a1=requant_vec(a1,Q6_V_vsplat_R(recip[bb+1]),zsh);
      a2=requant_vec(a2,Q6_V_vsplat_R(recip[bb+2]),zsh); a3=requant_vec(a3,Q6_V_vsplat_R(recip[bb+3]),zsh);
      a4=requant_vec(a4,Q6_V_vsplat_R(recip[bb+4]),zsh); a5=requant_vec(a5,Q6_V_vsplat_R(recip[bb+5]),zsh);
      a6=requant_vec(a6,Q6_V_vsplat_R(recip[bb+6]),zsh); a7=requant_vec(a7,Q6_V_vsplat_R(recip[bb+7]),zsh);
      *(HVX_Vector*)(g4s+(long)(2*sg)*128)=G4(a0,a1,a2,a3); *(HVX_Vector*)(g4s+(long)(2*sg+1)*128)=G4(a4,a5,a6,a7);
    }
    #undef G4
    HVX_Vector g0=*(HVX_Vector*)(g4s+0*128),g1=*(HVX_Vector*)(g4s+1*128),g2=*(HVX_Vector*)(g4s+2*128),g3=*(HVX_Vector*)(g4s+3*128),
               g5v=*(HVX_Vector*)(g4s+5*128),g4v=*(HVX_Vector*)(g4s+4*128),g6=*(HVX_Vector*)(g4s+6*128),g7=*(HVX_Vector*)(g4s+7*128);
    HVX_VectorPair p01=Q6_W_vshuff_VVR(g1,g0,-4),p23=Q6_W_vshuff_VVR(g3,g2,-4),p45=Q6_W_vshuff_VVR(g5v,g4v,-4),p67=Q6_W_vshuff_VVR(g7,g6,-4);
    HVX_VectorPair q0=Q6_W_vshuff_VVR(Q6_V_lo_W(p23),Q6_V_lo_W(p01),-8),q1=Q6_W_vshuff_VVR(Q6_V_hi_W(p23),Q6_V_hi_W(p01),-8),
                   q2=Q6_W_vshuff_VVR(Q6_V_lo_W(p67),Q6_V_lo_W(p45),-8),q3=Q6_W_vshuff_VVR(Q6_V_hi_W(p67),Q6_V_hi_W(p45),-8);
    HVX_VectorPair r0=Q6_W_vshuff_VVR(Q6_V_lo_W(q2),Q6_V_lo_W(q0),-16),r1=Q6_W_vshuff_VVR(Q6_V_hi_W(q2),Q6_V_hi_W(q0),-16),
                   r2=Q6_W_vshuff_VVR(Q6_V_lo_W(q3),Q6_V_lo_W(q1),-16),r3=Q6_W_vshuff_VVR(Q6_V_hi_W(q3),Q6_V_hi_W(q1),-16);
    if(npix==32){
      vstnt(orow+ 0*Cout,Q6_V_lo_W(r0)); vstnt(orow+ 4*Cout,Q6_V_hi_W(r0));
      vstnt(orow+ 8*Cout,Q6_V_lo_W(r1)); vstnt(orow+12*Cout,Q6_V_hi_W(r1));
      vstnt(orow+16*Cout,Q6_V_lo_W(r2)); vstnt(orow+20*Cout,Q6_V_hi_W(r2));
      vstnt(orow+24*Cout,Q6_V_lo_W(r3)); vstnt(orow+28*Cout,Q6_V_hi_W(r3));
    } else { HVX_Vector rr[8]={Q6_V_lo_W(r0),Q6_V_hi_W(r0),Q6_V_lo_W(r1),Q6_V_hi_W(r1),Q6_V_lo_W(r2),Q6_V_hi_W(r2),Q6_V_lo_W(r3),Q6_V_hi_W(r3)};
      if((npix&3)==0){ for(int j=0; j*4<npix; j++) *(HVX_UVec*)(orow+(long)j*4*Cout)=rr[j]; }
      else { for(int j=0;j<8;j++) *(HVX_Vector*)(g4s+(long)j*128)=rr[j]; __builtin_memcpy(orow, g4s, (long)npix*Cout); } }
  }
}
static void stem_conv_pil(unsigned char* out, unsigned char* in, signed char* wi8, int* bias, int* recip,
   int Cin,int H,int W,int Cout,int k,int stride,int pad,int zsh,int Ho,int Wo,int oy0,int oy1, unsigned char* cp4, int xzp){
  const int* wq = (const int*)wi8;
  for(int oy=oy0; oy<oy1; oy++){
    for(int ky=0; ky<k; ky++) stem_pil_fill(cp4+(long)ky*WP_PIL*4, in, oy*stride-pad+ky, H,W,Cin,pad,xzp);
    if(oy+1<oy1){ unsigned rb=(unsigned)(W*Cin)/128;
      for(int r=2;r<=3;r++){ int ii=oy*stride+r; if(ii>=0&&ii<H && rb) l2f(in+(long)ii*W*Cin,128,rb); } }
    stem_pil_out_row(out+(long)oy*Wo*Cout, cp4, wq, bias, recip, Wo, Cout, k, zsh);
  }
}
typedef struct { unsigned char *out,*in; int *wvec,*bias,*recip; int Cin,H,W,Cout,k,stride,pad,zsh,Ho,Wo,oy0,oy1,lock,vrmpy,xzp; unsigned char* cp4; } stemwork_t;
static void* stem_worker(void* arg){
  stemwork_t* w=arg;
  if(w->lock) qurt_hvx_lock(1);
  stem_conv_pil(w->out,w->in,(signed char*)w->wvec,w->bias,w->recip,w->Cin,w->H,w->W,w->Cout,w->k,w->stride,w->pad,w->zsh,w->Ho,w->Wo,w->oy0,w->oy1,w->cp4,w->xzp);
  if(w->lock) qurt_hvx_unlock();
  return 0;
}
__attribute__((always_inline)) static inline void head_blk(HVX_Vector cv, HVX_Vector rv, const HVX_Vector* mcv,
    HVX_Vector* ap, HVX_Vector vzc, HVX_Vector vzr, HVX_VecW vmr, HVX_Vector vz, int hasres){
  HVX_VectorPair hh=Q6_Wuh_vzxt_Vub(cv);
  HVX_VectorPair l0=Q6_Wuw_vzxt_Vuh(Q6_V_lo_W(hh)), l1=Q6_Wuw_vzxt_Vuh(Q6_V_hi_W(hh));
  HVX_Vector x[4]; x[0]=Q6_V_lo_W(l0); x[1]=Q6_V_hi_W(l0); x[2]=Q6_V_lo_W(l1); x[3]=Q6_V_hi_W(l1);
  HVX_Vector r[4];
  if(hasres){ HVX_VectorPair hr=Q6_Wuh_vzxt_Vub(rv);
    HVX_VectorPair r0=Q6_Wuw_vzxt_Vuh(Q6_V_lo_W(hr)), r1=Q6_Wuw_vzxt_Vuh(Q6_V_hi_W(hr));
    r[0]=Q6_V_lo_W(r0); r[1]=Q6_V_hi_W(r0); r[2]=Q6_V_lo_W(r1); r[3]=Q6_V_hi_W(r1); }
  for(int j=0;j<4;j++){
    HVX_VecW v = ((HVX_VecW)Q6_Vw_vsub_VwVw(x[j],vzc)) * ((HVX_VecW)mcv[j]);
    if(hasres) v = v + ((HVX_VecW)Q6_Vw_vsub_VwVw(r[j],vzr)) * vmr;
    ap[j] = Q6_Vw_vadd_VwVw(ap[j], Q6_Vw_vmax_VwVw((HVX_Vector)v, vz));
  }
}
static void head_gemm_range(float* emb, unsigned char* qu, signed char* gw, float* gws, float* gb, float* gwsum,
   int C, float z_bn, float s_bn, int o0, int o1){
#ifndef HEAD_PF_AHEAD
#define HEAD_PF_AHEAD 4
#endif
  int Cp=(C+127)&~127, gpf=Cp/128;
  for(int p=o0; p<o0+HEAD_PF_AHEAD && p<o1; p++) l2f(gw+(long)p*Cp, 128, gpf);
  for(int o=o0; o<o1; o++){
    if(o+HEAD_PF_AHEAD<o1) l2f(gw+(long)(o+HEAD_PF_AHEAD)*Cp, 128, gpf);
    float a = s_bn*((float)dot_u8i8(qu, gw+(long)o*Cp, C) - z_bn*gwsum[o]);
    emb[o] = gws[o]*a + gb[o]; }
}
typedef struct { float* emb; unsigned char* qu; signed char* gw; float *gws,*gb,*gwsum; int C; float z_bn,s_bn; int o0,o1; } headgemm_t;
static void* head_gemm_worker(void* a){ headgemm_t* w=a;
  head_gemm_range(w->emb,w->qu,w->gw,w->gws,w->gb,w->gwsum,w->C,w->z_bn,w->s_bn,w->o0,w->o1); return 0; }
typedef struct { float Asc,Asr,inv_sbn,z_bn,s_bn,inv2S; float *P,*Qb,*gws,*gb,*gwsum; int S,mr; } headp_t;
static headp_t head_prep(float* blob, int C, int O, int HW){
  headp_t h; h.Asc=blob[0]; h.Asr=blob[1]; h.inv_sbn=blob[2]; h.z_bn=blob[3]; h.s_bn=blob[4];
  h.P=blob+5; h.Qb=h.P+C; h.gws=h.Qb+C; h.gb=h.gws+O; h.gwsum=h.gb+O;
  float mx = h.Asc>h.Asr?h.Asc:h.Asr; if(mx<=0.0f) mx=1e-6f;
  h.S=30;
  while(h.S>0 && mx*(float)(1<<h.S) >= 1.0e6f) h.S--;
  while(h.S>0 && (float)HW*255.0f*(h.Asc+h.Asr)*(float)(1<<h.S) >= 2.0e9f) h.S--;
  h.inv2S = 1.0f/(float)(1<<h.S); h.mr = roundi(h.Asr*(float)(1<<h.S));
  return h;
}
static void head_tail(float* emb, const float* gap, const headp_t* h, signed char* gw, int C, int O, pool_t* pool,
    unsigned char* scratch){
  unsigned char* qu = SCR_TAKE(scratch, C);
  for(int c=0;c<C;c++){
    float bn=gap[c]*h->P[c]+h->Qb[c];
    int q=roundi(bn*h->inv_sbn)+(int)h->z_bn; if(q<0)q=0; if(q>255)q=255; qu[c]=(unsigned char)q; }
#ifndef HEAD_GEMM_TH_BYTES
#define HEAD_GEMM_TH_BYTES (384*1024)
#endif
  if((long)O*C >= HEAD_GEMM_TH_BYTES && pool && pool->up && O >= 64){
    headgemm_t w0={emb,qu,gw,h->gws,h->gb,h->gwsum,C,h->z_bn,h->s_bn,0,O/2},
               w1={emb,qu,gw,h->gws,h->gb,h->gwsum,C,h->z_bn,h->s_bn,O/2,O};
    pool_run2(pool, head_gemm_worker, &w0, &w1);
  } else head_gemm_range(emb, qu, gw, h->gws, h->gb, h->gwsum, C, h->z_bn, h->s_bn, 0, O);
}
static void head_gap_nhwc(float* gap, unsigned char* conv, unsigned char* res, float* gate, const headp_t* h,
    int C,int HW,int zc,int zr,int hasres,int has_gate,int relu, unsigned char* scratch){
  int nblk=C/128, Cv=nblk*128;
  int* mcp=SCR_TAKE(scratch, C*4); int* acc=SCR_TAKE(scratch, C*4);
  for(int c=0;c<Cv;c++){ int l=c&127,rr=l&3,gg=(rr==1?2:(rr==2?1:rr));
    mcp[(c&~127)+(gg<<5)+(l>>2)] = roundi(h->Asc*(has_gate?gate[c]:1.0f)*(float)(1<<h->S)); }
  HVX_Vector* ap=(HVX_Vector*)acc; HVX_Vector* mcv=(HVX_Vector*)mcp;
  for(int v=0;v<nblk*4;v++) ap[v]=Q6_V_vzero();
  HVX_Vector vzc=Q6_V_vsplat_R(zc), vzr=Q6_V_vsplat_R(zr);
  HVX_Vector vz=relu?Q6_V_vzero():Q6_V_vsplat_R(0x80000000);
  HVX_VecW vmr=(HVX_VecW)Q6_V_vsplat_R(h->mr);
  for(int i=0;i<HW;i++){
    HVX_UVec* ic=(HVX_UVec*)(conv+(long)i*C);
    HVX_UVec* ir=hasres?(HVX_UVec*)(res+(long)i*C):ic;
    for(int blk=0;blk<nblk;blk++)
      head_blk(ic[blk], hasres?ir[blk]:ic[blk], mcv+blk*4, ap+blk*4, vzc,vzr,vmr,vz, hasres);
  }
  for(int c=0;c<Cv;c++){ int l=c&127,rr=l&3,gg=(rr==1?2:(rr==2?1:rr));
    gap[c]=(float)acc[(c&~127)+(gg<<5)+(l>>2)]*h->inv2S; }
  const float vlof = relu ? 0.0f : -3.0e38f;
  for(int c=Cv;c<C;c++){
    float ag=h->Asc*(has_gate?gate[c]:1.0f), s=0.0f;
    for(int i=0;i<HW;i++){ float v=ag*((float)conv[(long)i*C+c]-zc);
      if(hasres) v+=h->Asr*((float)res[(long)i*C+c]-zr); if(v<vlof)v=vlof; s+=v; }
    gap[c]=s;
  }
}
static void head_gap_d32(float* restrict gap, unsigned char* restrict conv, unsigned char* restrict res, float* gate,
    const headp_t* h, int C,int Ho,int Wo,int Wop,int zc,int zr,int hasres,int has_gate,int relu){
  int nchunk=C/32; long rowstride=(long)nchunk*Wop*32; int wg=Wo>>2, wtail=Wo&3;
  HVX_Vector vzc=Q6_V_vsplat_R(zc), vzr=Q6_V_vsplat_R(zr);
  HVX_Vector vz=relu?Q6_V_vzero():Q6_V_vsplat_R(0x80000000);
  HVX_VecW vmr=(HVX_VecW)Q6_V_vsplat_R(h->mr);
  int a128[128] __attribute__((aligned(128))), mcp[128] __attribute__((aligned(128))), mcn[32];
  { long span=(long)Ho*rowstride; unsigned nb=(unsigned)(span/128); if(nb>65535) nb=65535;
    if(nb){ l2f(conv,128,nb); if(hasres) l2f(res,128,nb); } }
  for(int ch=0; ch<nchunk; ch++){
    for(int c=0;c<32;c++) mcn[c]=roundi(h->Asc*(has_gate?gate[ch*32+c]:1.0f)*(float)(1<<h->S));
    for(int b=0;b<128;b++){ int rr=b&3, gg=(rr==1?2:(rr==2?1:rr));
      mcp[(gg<<5)+(b>>2)] = mcn[b&31]; }
    HVX_Vector* mcv=(HVX_Vector*)mcp; HVX_Vector* ap=(HVX_Vector*)a128;
    for(int v=0;v<4;v++) ap[v]=Q6_V_vzero();
    for(int hh=0; hh<Ho; hh++){
      HVX_Vector* ic=(HVX_Vector*)(conv+(long)hh*rowstride+(long)ch*Wop*32);
      HVX_Vector* ir=hasres?(HVX_Vector*)(res+(long)hh*rowstride+(long)ch*Wop*32):ic;
      for(int g=0; g<wg; g++) head_blk(ic[g], hasres?ir[g]:ic[g], mcv, ap, vzc,vzr,vmr,vz, hasres);
    }
    int csum[32];
    for(int c=0;c<32;c++){
      int sv=0;
      for(int slot=0;slot<4;slot++){ int b=slot*32+c, rr=b&3, gg=(rr==1?2:(rr==2?1:rr)); sv+=a128[(gg<<5)+(b>>2)]; }
      csum[c]=sv;
    }
    if(wtail){ unsigned char* pc0=conv+(long)ch*Wop*32; unsigned char* pr0=hasres?res+(long)ch*Wop*32:pc0;
      for(int c=0;c<32;c++){ int sv=csum[c], m=mcn[c];
        for(int hh=0;hh<Ho;hh++){ unsigned char* pc=pc0+(long)hh*rowstride; unsigned char* pr=pr0+(long)hh*rowstride;
          for(int w=wg*4;w<Wo;w++){ int vv=((int)pc[w*32+c]-zc)*m;
            if(hasres) vv+=((int)pr[w*32+c]-zr)*h->mr; if(vv<0)vv=0; sv+=vv; } }
        csum[c]=sv; } }
    for(int c=0;c<32;c++) gap[ch*32+c]=(float)csum[c]*h->inv2S;
  }
}

extern void dwconv2dbbb_s1_3x3_asm(unsigned char* in, unsigned char* filt, unsigned char* out,
   int next_in_width, int next_out_width, int next_in_width_32, int next_out_width_32,
   int depth, int out_width, int out_height, int filt_width, int filt_zero,
   int* bias_sum, int* max, int* recip_level, int recip_shift, int stride_height);
extern void dwconv2dbbb_s1_5xN_asm(unsigned char* in, unsigned char* filt, unsigned char* out,
   int next_in_width, int next_out_width, int next_in_width_32, int next_out_width_32,
   int depth, int out_width, int out_height, int filt_height, int filt_zero,
   int* bias_sum, int* max, int* recip_level, int recip_shift, int stride_height);
extern void dwconv2dbbb_s1_7xN_asm(unsigned char* in, unsigned char* filt, unsigned char* out,
   int next_in_width, int next_out_width, int next_in_width_32, int next_out_width_32,
   int depth, int out_width, int out_height, int filt_width, int filt_zero,
   int* bias_sum, int* max, int* recip_level, int recip_shift, int stride_height, void* sbuf);
extern void dwconv2dbbb_s2_3x3_asm(unsigned char* in, unsigned char* filt, unsigned char* out,
   int next_in_width, int next_out_width, int next_in_width_32, int next_out_width_32,
   int depth, int out_width, int out_height, int filt_width, int filt_zero,
   int* bias_sum, int* max, int* recip_level, int recip_shift, int stride_height, int filler, int in_left_skip);
extern void dwconv2dbbb_s2_5xN_asm(unsigned char* in, unsigned char* filt, unsigned char* out,
   int next_in_width, int next_out_width, int next_in_width_32, int next_out_width_32,
   int depth, int out_width, int out_height, int filt_height, int filt_zero,
   int* bias_sum, int* max, int* recip_level, int recip_shift, int stride_height);
#ifndef DW_TH
#define DW_TH 4
#endif
typedef struct { unsigned char *d32in,*d32out,*out,*filt; int *bias,*recipv,*mm;
  int niw,niw32,now,now32,C,oW,owt,oH,kw,kh,fz,rsh,s,ils,oLp,D,outd,realC,kern,tile; void* sbuf; int oy0,oy1,lock;
  unsigned long long* prof; } dwwork_t;
static void* dw_worker(void* arg){
  dwwork_t* w=arg;
  if(w->oy0>=w->oy1) return 0;
  int* mm=w->mm; int* recipv=w->recipv;
  if(w->lock) qurt_hvx_lock(1);
  for(int i=0;i<32;i++){ mm[i]=-0x7fffffff; mm[32+i]=0x7fffffff; }
  for(int oy0=w->oy0; oy0<w->oy1; oy0+=w->tile){
    int nh=(w->oy1-oy0<w->tile)?(w->oy1-oy0):w->tile, oyn=oy0+nh;
    if(oyn<w->oH){ int nhn=(w->oH-oyn<DW_TH)?(w->oH-oyn):DW_TH;
      l2f(w->d32in+(long)oyn*w->s*w->niw, 128, (unsigned)((long)(nhn*w->s+w->kh)*w->niw/128)); }
    unsigned char* ti=w->d32in+(long)oy0*w->s*w->niw; unsigned char* to=w->d32out+(long)oy0*w->now;
    int ow_k = w->oLp ? w->owt : w->oW;
    switch(w->kern){
      case 1: dwconv2dbbb_s1_3x3_asm(ti,w->filt,to,w->niw,w->now,w->niw32,w->now32,w->C,ow_k,nh,w->kw,w->fz,w->bias,mm,recipv,w->rsh,1); break;
      case 2: dwconv2dbbb_s1_5xN_asm(ti,w->filt,to,w->niw,w->now,w->niw32,w->now32,w->C,ow_k,nh,w->kh,w->fz,w->bias,mm,recipv,w->rsh,1); break;
      case 3: dwconv2dbbb_s1_7xN_asm(ti,w->filt,to,w->niw,w->now,w->niw32,w->now32,w->C,ow_k,nh,w->kh,w->fz,w->bias,mm,recipv,w->rsh,1,w->sbuf); break;
      case 4: dwconv2dbbb_s2_3x3_asm(ti,w->filt,to,w->niw,w->now,w->niw32,w->now32,w->C,w->owt,nh,w->kw,w->fz,w->bias,mm,recipv,w->rsh,2,0,w->ils); break;
      case 5: dwconv2dbbb_s2_5xN_asm(ti,w->filt,to,w->niw,w->now,w->niw32,w->now32,w->C,ow_k,nh,w->kh,w->fz,w->bias,mm,recipv,w->rsh,2); break;
    }
    unsigned long long* prof=w->prof;
    PHASE(PH_DW_UNPACK) if(!w->outd){ unsigned char* tv=to+(long)w->oLp*32; int rc=w->realC;
      from_d32_asm(tv, w->owt*32, w->out+(long)oy0*w->oW*rc, w->oW, nh, rc); }
  }
  if(w->lock) qurt_hvx_unlock();
  return 0;
}
static void dwconv_op(unsigned char* arena, unsigned char* filt, int* bias, int* minmax, int* op, pool_t* pool,
   unsigned long long* prof){
  (void)minmax;
  int ind=op[DW_ind], outd=op[DW_outd], ind_bordered=op[DW_ind_bord];
  unsigned char* out=arena+op[DW_out]; unsigned char* in=arena+op[DW_src];
  unsigned char* d32in=arena+op[DW_d32in]; unsigned char* d32out=outd?(arena+op[DW_out]):(arena+op[DW_d32out]);
  int C=op[DW_C],H=op[DW_H],W=op[DW_W],kh=op[DW_kh],kw=op[DW_kw],fz=op[DW_fz],recip=op[DW_recip],rsh=op[DW_rsh];
  int s=op[DW_s]?op[DW_s]:1;
  int Cp=op[DW_Cp], pad=op[DW_pad], ofw=op[DW_ofw], padL=op[DW_padL], Wp=op[DW_Wp], Hp=op[DW_Hp];
  int xzp=op[DW_xzp];
  int oH=op[DW_oH], oW=op[DW_oW], oLp=op[DW_oLp], ils=op[DW_ils], owt=op[DW_owt];
  int D=Cp/32;
  PHASE(PH_DW_REPAD)
  if(ind_bordered){
    long rowsz=(long)D*Wp*32;
    d32_fill(d32in, xzp, (long)pad*rowsz);
    d32_fill(d32in+(long)(pad+H)*rowsz, xzp, (long)(Hp-pad-H)*rowsz);
    for(int h=0;h<H;h++){ unsigned char* row=d32in+(long)(h+pad)*rowsz;
      for(int d=0;d<D;d++){ unsigned char* ds=row+(long)d*(Wp*32);
        d32_fill(ds, xzp, (long)padL*32);
        d32_fill(ds+(long)(padL+W)*32, xzp, (long)(Wp-padL-W)*32); } }
  } else if(ind){
    int iwp2=op[DW_src_wop]?op[DW_src_wop]:((W+3)&(~3)); long rowsz=(long)D*Wp*32;
    d32_fill(d32in, xzp, (long)pad*rowsz);
    d32_fill(d32in+(long)(pad+H)*rowsz, xzp, (long)(Hp-pad-H)*rowsz);
    for(int h=0;h<H;h++){ unsigned char* row=d32in+(long)(h+pad)*rowsz;
      for(int d=0;d<D;d++){ unsigned char* ds=row+(long)d*(Wp*32);
        d32_fill(ds, xzp, (long)padL*32);
        d32_copy(ds+(long)padL*32, in+(long)h*(D*iwp2*32)+(long)d*(iwp2*32), (long)W*32);
        d32_fill(ds+(long)(padL+W)*32, xzp, (long)(Wp-padL-W)*32); } }
  }
  int niw=D*Wp*32, niw32=Wp*32, now=D*owt*32, now32=owt*32;
  { long fb=(long)D*kh*ofw*32; if(fb>=128) l2f(filt,128,(unsigned)(fb/128)); }
  int aux = D*128;
  int* recipv = (int*)(arena+op[DW_aux]);
  int* mm     = (int*)(arena+op[DW_aux]+aux);
  for(int i=0;i<32*D;i++) recipv[i]=recip;
  for(int i=0;i<32;i++){ mm[i]=-0x7fffffff; mm[32+i]=0x7fffffff; }
  {
    int tile=op[DW_tile]?op[DW_tile]:DW_TH, threads=op[DW_threads]?op[DW_threads]:1;
    void* sbuf = arena+op[DW_aux]+aux+256;
    int tile0 = oH<tile?oH:tile;
    l2f(d32in, 128, (unsigned)((long)(tile0*s+kh)*niw/128));
    dwwork_t base = { d32in,d32out,out,filt,bias,recipv,mm, niw,niw32,now,now32,Cp,oW,owt,oH,kw,kh,fz,rsh,s,ils,oLp,D,outd,C,op[DW_kern],tile, sbuf, 0,oH,0, prof };
    PHASE(PH_DW_TILE) if(threads==2){
      int tiles=(oH+tile-1)/tile, rt=((tiles+1)/2)*tile;
      dwwork_t d0=base, d1=base;
      d0.oy0=0;  d0.oy1=rt<oH?rt:oH; d0.mm=(int*)(arena+op[DW_aux]+aux);
      d1.oy0=rt<oH?rt:oH; d1.oy1=oH; d1.mm=(int*)(arena+op[DW_aux]+aux+256);
      pool_run2(pool, dw_worker, &d0, &d1);
    } else dw_worker(&base);
  }
}

__attribute__((noinline)) void interp(unsigned char* arena, unsigned char* wts, int* ops, int nops,
   unsigned char* d32in, unsigned char* d32out, int* minmax, unsigned long long* prof){
  pool_t pool; pool.up=0;
  int has_work=0;
  for(int i=0;i<nops;i++){ int o=ops[i*INTS_PER_OP]; if(o==OP_DWCONV||o==OP_CONV){ has_work=1; break; } }
  if(has_work) pool_start(&pool);
  for(int i=0;i<nops;i++){
    int* op = ops + i*INTS_PER_OP;
    unsigned long long t0=HAP_perf_get_time_us();
#ifndef CROSS_PF_KB
#define CROSS_PF_KB 64
#endif
#if CROSS_PF_KB>0
    if(i+1<nops){ int* nx=ops+(i+1)*INTS_PER_OP; int o=nx[0];
      if(o==OP_CONV||o==OP_DWCONV||o==OP_INCONV){ L2FP(prof, wts+nx[3], 128, (CROSS_PF_KB*1024u)/128); } }
#endif
    if(op[0]==OP_CONV){
      conv_op(arena+op[CV_src], arena+op[CV_out], d32in, d32out, wts+op[CV_wt],
              (int*)(wts+op[CV_bias]), (int*)(wts+op[CV_recip]), minmax, op+CV_P0, prof, arena, &pool);
    } else if(op[0]==OP_SE_GATE){
      se_gate((float*)(arena+op[SE_gate]), arena+op[SE_expand], (signed char*)(wts+op[SE_fc1w]),
              (signed char*)(wts+op[SE_fc2w]), (float*)(wts+op[SE_blob]), (float*)(wts+op[SE_lut]),
              op[SE_Cexp],op[SE_Csq], op[SE_eH],op[SE_eW],op[SE_eWop], d32in, prof);
    } else if(op[0]==OP_SETAIL){
      setail_d32(arena+op[ST_out], arena+op[ST_conv], arena+op[ST_res], (float*)(arena+op[ST_gate]),
             (float*)(wts+op[ST_blob]), op[ST_C],op[ST_H],op[ST_W],op[ST_Wop],
             op[ST_zc],op[ST_zr],op[ST_zo],op[ST_relu],op[ST_hasres], op[ST_has_gate], op[ST_outd32], d32in);
    } else if(op[0]==OP_HEAD){
      int hC=op[HD_C], hO=op[HD_O], hHW=op[HD_HW];
      unsigned char* hs=d32in; float* gap=SCR_TAKE(hs, hC*4);
      headp_t hp = head_prep((float*)(wts+op[HD_blob]), hC, hO, hHW);
      if(op[HD_d32])
        head_gap_d32(gap, arena+op[HD_conv], arena+op[HD_res], (float*)(arena+op[HD_gate]), &hp,
           hC,op[HD_H],op[HD_W],op[HD_Wop],op[HD_zcc],op[HD_zr],op[HD_hasres],op[HD_has_gate],op[HD_relu]);
      else
        head_gap_nhwc(gap, arena+op[HD_conv], arena+op[HD_res], (float*)(arena+op[HD_gate]), &hp,
           hC,hHW,op[HD_zcc],op[HD_zr],op[HD_hasres],op[HD_has_gate],op[HD_relu], hs);
      head_tail((float*)(arena+op[HD_out]), gap, &hp, (signed char*)(wts+op[HD_gw]), hC, hO, &pool, hs);
    } else if(op[0]==OP_DWCONV){
      dwconv_op(arena, wts+op[DW_filt], (int*)(wts+op[DW_bias]), minmax, op, &pool, prof);
    } else if(op[0]==OP_ADD){
      ewacc_t st[2]={{arena+op[ADD_a],op[ADD_za],op[ADD_ra]},{arena+op[ADD_b],op[ADD_zb],op[ADD_rb]}};
      ewise_op(arena+op[ADD_out], st, 2, op[ADD_S], op[ADD_zo], op[ADD_qmax], (long)op[ADD_n]);
    } else if(op[0]==OP_PACK){
      pack_op(arena+op[PK_src], op[PK_W], arena+op[PK_out], op[PK_Wop]*32, op[PK_H], op[PK_Cigp], prof);
    } else if(op[0]==OP_INCONV){
      int inHo=op[IC_Ho];
      stemwork_t sb={arena+op[IC_out], arena+op[IC_src], (int*)(wts+op[IC_wvec]), (int*)(wts+op[IC_bias]),
                     (int*)(wts+op[IC_recip]), op[IC_Cin],op[IC_H],op[IC_W],op[IC_Cout],op[IC_k],op[IC_stride],
                     op[IC_pad],op[IC_zsh],op[IC_Ho],op[IC_Wo],0,inHo,0,2};
      sb.xzp=op[IC_xzp];
      int split=inHo/2;
      stemwork_t s0=sb, s1=sb; s0.oy0=0; s0.oy1=split; s1.oy0=split; s1.oy1=inHo;
      s0.cp4=arena+op[IC_cp4]; s1.cp4=arena+op[IC_cp4]+3*WP_PIL*4;
      pool_run2(&pool, stem_worker, &s0, &s1);
    }
    unsigned long long dt=HAP_perf_get_time_us()-t0;
    if(prof){ prof[op[0]] += dt; if(i<220) prof[16+i]=dt; }
  }
  if(pool.up) pool_stop(&pool);
}

#ifdef PF_CAL
#include "pfcal.h"
#endif

#ifndef INTS_PER_OP
#error "define the op-record contract: INTS_PER_OP / OP_CONV / OP_SE_GATE / OP_SETAIL / OP_HEAD (codegen.opcode_defines)"
#endif
enum { HDR_NOPS, HDR_SEED_OFF, HDR_SEED_BYTES, HDR_OUT_OFF, HDR_OUT_BYTES,
       HDR_D32IN, HDR_D32OUT, HDR_MINMAX };
static void interp_kernel(float* out, unsigned char* seed, unsigned char* arena, unsigned char* wts, int* ops,
                          unsigned char* scratch){
  const int* h = ops;
  int nops = h[HDR_NOPS], out_bytes = h[HDR_OUT_BYTES];
  unsigned char* d32in  = scratch + h[HDR_D32IN];
  unsigned char* d32out = scratch + h[HDR_D32OUT];
  int*           minmax = (int*)(scratch + h[HDR_MINMAX]);
  ops += INTS_PER_OP;
  __builtin_memcpy(arena + h[HDR_SEED_OFF], seed, h[HDR_SEED_BYTES]);
#ifdef PROF_OPS
  unsigned long long* prof = (unsigned long long*)out;
  int _pn = out_bytes/8; if(_pn>256) _pn=256; for(int i=0;i<_pn;i++) prof[i]=0;
#ifdef PF_CAL
  pf_calibrate(wts, d32in, prof); return;
#endif
  interp(arena, wts, ops, nops, d32in, d32out, minmax, prof);
#else
  interp(arena, wts, ops, nops, d32in, d32out, minmax, 0);
  __builtin_memcpy(out, arena + h[HDR_OUT_OFF], out_bytes);
#endif
}
int entry(unsigned long long handle, unsigned int sc, remote_arg* pra) {
  HAP_power_request_t req; for(unsigned i=0;i<sizeof(req);i++)((char*)&req)[i]=0;
  req.type=HAP_power_set_DCVS_v2;
  req.dcvs_v2.dcvs_enable=0; req.dcvs_v2.dcvs_option=HAP_DCVS_V2_PERFORMANCE_MODE;
  req.dcvs_v2.set_latency=1; req.dcvs_v2.latency=100; req.dcvs_v2.set_dcvs_params=1;
  req.dcvs_v2.dcvs_params.max_corner=CORNER; req.dcvs_v2.dcvs_params.min_corner=CORNER; req.dcvs_v2.dcvs_params.target_corner=CORNER;
  HAP_power_set(0,&req);
  HAP_power_request_t bw; for(unsigned i=0;i<sizeof(bw);i++)((char*)&bw)[i]=0;
  bw.type=HAP_power_set_mips_bw; bw.mips_bw.set_mips=1; bw.mips_bw.mipsPerThread=1000; bw.mips_bw.mipsTotal=4000;
  bw.mips_bw.set_bus_bw=1; bw.mips_bw.bwBytePerSec=12000000000ULL; bw.mips_bw.busbwUsagePercentage=100;
  bw.mips_bw.set_latency=1; bw.mips_bw.latency=1;
  HAP_power_set((void*)handle,&bw);
  if ((sc>>24) != 2) return 0;
  int na = ((int*)pra[0].buf.pv)[0];
  void *bases[16];
  for (int j = 0; j < na; j++) bases[j] = HAP_mmap(0, ((int*)pra[0].buf.pv)[1+j], 3, 0, pra[3+j].dma.fd, 0);
  void *b[6];
  for (int i = 0; i < 6; i++) b[i] = (char*)bases[((int*)pra[1].buf.pv)[i*2]] + ((int*)pra[1].buf.pv)[i*2+1];
  unsigned long long start = HAP_perf_get_time_us();
  interp_kernel((float*)b[0], (unsigned char*)b[1], (unsigned char*)b[2], (unsigned char*)b[3], (int*)b[4], (unsigned char*)b[5]);
  *(unsigned long long *)(pra[2].buf.pv) = HAP_perf_get_time_us() - start;
  for (int j = 0; j < na; j++) HAP_munmap(bases[j], ((int*)pra[0].buf.pv)[1+j]);
  return 0;
}
