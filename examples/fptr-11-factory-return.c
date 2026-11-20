// fptr-11-factory-return.c
//
// Fact F11: a function returns a function pointer (handler factory /
// lookup).  Requires a return-target summary: pick() returns {use_int}.
//
// Real world: ImageMagick's coder accessor
//   MagickExport DecodeImageHandler *GetImageDecoder(const MagickInfo *)
// (test_projects/bench-3c/ImageMagick-7.0.11-6/MagickCore/magick.c:223)
// returns the decoder function pointer stored in a MagickInfo record.
//
// @fptr-fact F11 factory-return
// @subject   run::ctx
// @baseline  category=OpaqueTransportOrViewOnly callback=UnresolvedCallback behavior=CallbackDependentPointer types=-
// @expected  category=MonomorphicActiveSpecialization types=int* resolution=exact@L1
typedef int (*handler_t)(void *);

static int use_int(void *p) { int *q = p; return *q; }

static handler_t pick(void) { return use_int; }

int run(void *ctx) {
  handler_t cb = pick();
  return cb(ctx);
}
