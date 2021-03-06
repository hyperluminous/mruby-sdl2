#include "sdl2_rwops.h"
#include "mruby/class.h"
#include "mruby/data.h"
#include "mruby/string.h"


static struct RClass *class_RWops = NULL;


typedef struct mrb_sdl2_rwops_data_t {
  bool        is_associated;
  SDL_RWops *rwops;
} mrb_sdl2_rwops_data_t;

static void
mrb_sdl2_rwops_data_free(mrb_state *mrb, void *p)
{
  mrb_sdl2_rwops_data_t *data =
    (mrb_sdl2_rwops_data_t*)p;
  if (NULL != data) {
    if ((!data->is_associated) && (NULL != data->rwops)) {
      SDL_FreeRW(data->rwops);
    }
    mrb_free(mrb, data);
  }
}

static struct mrb_data_type const mrb_sdl2_rwops_data_type = {
  "RWops", &mrb_sdl2_rwops_data_free
};

SDL_RWops *
mrb_sdl2_rwops_get_ptr(mrb_state *mrb, mrb_value rwops)
{
  mrb_sdl2_rwops_data_t *data;
  if (mrb_nil_p(rwops)) {
    return NULL;
  }

  data =
    (mrb_sdl2_rwops_data_t*)mrb_data_get_ptr(mrb, rwops, &mrb_sdl2_rwops_data_type);
  return data->rwops;
}

mrb_value
mrb_sdl2_rwops(mrb_state *mrb, SDL_RWops *rwops)
{
  mrb_sdl2_rwops_data_t *data;
  if (NULL == rwops) {
    return mrb_nil_value();
  }

  data =
    (mrb_sdl2_rwops_data_t*)mrb_malloc(mrb, sizeof(mrb_sdl2_rwops_data_t));
  if (NULL == data) {
    mrb_raise(mrb, E_RUNTIME_ERROR, "insufficient memory.");
  }
  data->is_associated = false;
  data->rwops = rwops;
  return mrb_obj_value(Data_Wrap_Struct(mrb, class_RWops, &mrb_sdl2_rwops_data_type, data));
}

mrb_value
mrb_sdl2_rwops_associated_rwops(mrb_state *mrb, SDL_RWops *rwops)
{
  mrb_sdl2_rwops_data_t *data;
  if (NULL == rwops) {
    return mrb_nil_value();
  }

  data =
    (mrb_sdl2_rwops_data_t*)mrb_malloc(mrb, sizeof(mrb_sdl2_rwops_data_t));
  if (NULL == data) {
    mrb_raise(mrb, E_RUNTIME_ERROR, "insufficient memory.");
  }
  data->is_associated = true;
  data->rwops = rwops;
  return mrb_obj_value(Data_Wrap_Struct(mrb, class_RWops, &mrb_sdl2_rwops_data_type, data));
}



/***************************************************************************
*
* module SDL2::RWops::RWops
*
***************************************************************************/


static mrb_value
mrb_sdl2_rwops_initialize(mrb_state *mrb, mrb_value self)
{
  SDL_RWops *rwops = NULL;
  mrb_sdl2_rwops_data_t *data =
    (mrb_sdl2_rwops_data_t*)DATA_PTR(self);
  if (NULL == data) {
    data = (mrb_sdl2_rwops_data_t*)mrb_malloc(mrb, sizeof(mrb_sdl2_rwops_data_t));
    if (NULL == data) {
      mrb_raise(mrb, E_RUNTIME_ERROR, "insufficient memory.");
    }
    data->is_associated = false;
    data->rwops = NULL;
  }

  if (2 == mrb->c->ci->argc) {
    mrb_value file, mode;
    mrb_get_args(mrb, "SS", &file, &mode);
    rwops = SDL_RWFromFile(RSTRING_PTR(file), RSTRING_PTR(mode));
  } else {
    mrb_free(mrb, data);
    mrb_raise(mrb, E_ARGUMENT_ERROR, "wrong number of arguments.");
  }
  if (NULL == rwops) {
    mrb_free(mrb, data);
    mruby_sdl2_raise_error(mrb);
  }

  data->rwops = rwops;

  DATA_PTR(self) = data;
  DATA_TYPE(self) = &mrb_sdl2_rwops_data_type;
  return self;
}

static mrb_value
mrb_sdl2_rwops_free(mrb_state *mrb, mrb_value self)
{
  mrb_sdl2_rwops_data_t *data =
    (mrb_sdl2_rwops_data_t*)mrb_data_get_ptr(mrb, self, &mrb_sdl2_rwops_data_type);
  if ((!data->is_associated) && (NULL != data->rwops)) {
    SDL_FreeRW(data->rwops);
    data->rwops = NULL;
  }
  return self;
}

static mrb_value
mrb_sdl2_rwops_size(mrb_state *mrb, mrb_value self)
{
  SDL_RWops * rwops = mrb_sdl2_rwops_get_ptr(mrb, self);
  Sint64 result = SDL_RWsize(rwops);

  if (-1 > result)
    mruby_sdl2_raise_error(mrb);

  return mrb_fixnum_value(result);
}

static mrb_value
mrb_sdl2_rwops_seek(mrb_state *mrb, mrb_value self)
{
  SDL_RWops * rwops = mrb_sdl2_rwops_get_ptr(mrb, self);
  mrb_int offset, whence;
  Sint64 result;
  mrb_get_args(mrb, "ii", &offset, &whence);
  result = SDL_RWseek(rwops, (Sint64) offset, (int) whence);
  return mrb_fixnum_value(result);
}

static mrb_value
mrb_sdl2_rwops_tell(mrb_state *mrb, mrb_value self)
{
  SDL_RWops * rwops = mrb_sdl2_rwops_get_ptr(mrb, self);
  Sint64 result = SDL_RWtell(rwops);
  return mrb_fixnum_value(result);
}

static mrb_value
mrb_sdl2_rwops_read(mrb_state *mrb, mrb_value self)
{
    mrb_int size;
    mrb_value result;
    SDL_RWops * rw = mrb_sdl2_rwops_get_ptr(mrb, self);
    mrb_get_args(mrb, "i", &size);

    result = mrb_str_new(mrb, NULL, size);
    SDL_RWread(rw, RSTRING_PTR(result), size, 1);

    return result;
}

static mrb_value
mrb_sdl2_rwops_write(mrb_state *mrb, mrb_value self)
{
  SDL_RWops * rwops;
  mrb_value ptr, buf;
  mrb_get_args(mrb, "S", &ptr);

  if (mrb_type(ptr) != MRB_TT_STRING) {
    buf = mrb_funcall(mrb, ptr, "to_s", 0);
  } else {
    buf = ptr;
  }

  rwops = mrb_sdl2_rwops_get_ptr(mrb, self);
  if(rwops != NULL) {
      char *str = RSTRING_PTR(buf);
      size_t len = SDL_strlen(str);
      if (SDL_RWwrite(rwops, str, 1, len) != len) {
          mruby_sdl2_raise_error(mrb);
      }
  }
  return mrb_true_value();
}

static mrb_value
mrb_sdl2_rwops_close(mrb_state *mrb, mrb_value self)
{

  SDL_RWops * rwops = mrb_sdl2_rwops_get_ptr(mrb, self);
  int result = SDL_RWclose(rwops);
  if (result == -1) {
    mruby_sdl2_raise_error(mrb);
  }
  return mrb_true_value();
}

static mrb_value
mrb_sdl2_rwops_file_exists(mrb_state *mrb, mrb_value self)
{
  mrb_value path;
  SDL_RWops* rwops;
  Uint8 result = 0;
  mrb_get_args(mrb, "S", &path);
  rwops = SDL_RWFromFile(RSTRING_PTR(path), "r");
  if (rwops != NULL) {
    result = 1;
    SDL_RWclose(rwops);
  } else {
    result = 0;
  }
  return result == 0 ? mrb_false_value() : mrb_true_value();
}

void
mruby_sdl2_rwops_init(mrb_state *mrb)
{
  int arena_size;
  struct RClass *class_RWops = mrb_define_class_under(mrb, mod_SDL2, "RWops", mrb->object_class);

  MRB_SET_INSTANCE_TT(class_RWops, MRB_TT_DATA);

  mrb_define_method(mrb, class_RWops, "initialize", mrb_sdl2_rwops_initialize, MRB_ARGS_REQ(2));
  mrb_define_method(mrb, class_RWops, "destroy",    mrb_sdl2_rwops_free,       MRB_ARGS_NONE());
  mrb_define_method(mrb, class_RWops, "free",       mrb_sdl2_rwops_free,       MRB_ARGS_NONE());
  mrb_define_method(mrb, class_RWops, "size",       mrb_sdl2_rwops_size,       MRB_ARGS_NONE());
  mrb_define_method(mrb, class_RWops, "seek",       mrb_sdl2_rwops_seek,       MRB_ARGS_REQ(2));
  mrb_define_method(mrb, class_RWops, "tell",       mrb_sdl2_rwops_tell,       MRB_ARGS_NONE());
  mrb_define_method(mrb, class_RWops, "read",       mrb_sdl2_rwops_read,       MRB_ARGS_REQ(2));
  mrb_define_method(mrb, class_RWops, "write",      mrb_sdl2_rwops_write,      MRB_ARGS_REQ(1));
  mrb_define_method(mrb, class_RWops, "close",      mrb_sdl2_rwops_close,      MRB_ARGS_NONE());

  mrb_define_module_function(mrb, class_RWops, "file_exists?", mrb_sdl2_rwops_file_exists, MRB_ARGS_REQ(1));


  arena_size = mrb_gc_arena_save(mrb);
  mrb_define_const(mrb, class_RWops, "RW_SEEK_SET", mrb_fixnum_value(RW_SEEK_SET));
  mrb_define_const(mrb, class_RWops, "RW_SEEK_CUR", mrb_fixnum_value(RW_SEEK_CUR));
  mrb_define_const(mrb, class_RWops, "RW_SEEK_END", mrb_fixnum_value(RW_SEEK_END));
  mrb_gc_arena_restore(mrb, arena_size);
}

void
mruby_sdl2_rwops_final(mrb_state *mrb)
{
}
