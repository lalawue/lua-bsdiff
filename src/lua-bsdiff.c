/*
 * Copyright (c) 2021 lalawue
 * 
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 */

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

#include "bsdiff.h"
#include "bspatch.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define BUF_LEN (256 * 1024)
#define MIN_LEN(X, Y) ((X) < (Y) ? (X) : (Y))

static const char *header_str = "ENDSLEY/BSDIFF43";

/* check types in lua stack */
static int
_check_type(lua_State *L, int *types, int count)
{
   for (int i = 0; i < count; i++)
   {
      if (lua_type(L, i + 1) != types[i])
      {
         return 0;
      }
   }
   return 1;
}

struct s_buf
{
   uint8_t buf[BUF_LEN];
   int len;
   struct s_buf *next;
};

struct s_head
{
   struct s_buf *head;
   struct s_buf *tail;
};

static struct s_buf *
_get_buf(struct s_head *h)
{
   if (h->tail == NULL)
   {
      struct s_buf *b = (struct s_buf *)malloc(sizeof(struct s_buf));
      b->len = 0;
      b->next = NULL;
      h->head = h->tail = b;
   }
   else
   {
      if (h->tail->len >= BUF_LEN)
      {
         struct s_buf *b = (struct s_buf *)malloc(sizeof(struct s_buf));
         b->len = 0;
         b->next = NULL;
         h->tail->next = b;
         h->tail = b;
      }
   }
   return h->tail;
}

static void
_free_buf(struct s_head *h)
{
   if (h->head)
   {
      struct s_buf *p, *n;
      n = h->head;
      do
      {
         p = n;
         n = p->next;
         free(p);
      } while (n);
   }
}

static int
_write_cb(struct bsdiff_stream *stream, const void *buffer, int size)
{
   while (size > 0)
   {
      struct s_buf *b = _get_buf(stream->opaque);
      int len = MIN_LEN((BUF_LEN - b->len), size);
      memcpy(&b->buf[b->len], buffer, len);
      b->len += len;
      size -= len;
   }
   return 0;
}

static int
_ldiff(lua_State *L)
{
   int types[2] = {LUA_TSTRING, LUA_TSTRING};
   if (!_check_type(L, types, 2))
   {
      lua_pushboolean(L, 0);
      lua_pushstring(L, "invalid params");
      return 2;
   }

   size_t old_sz = 0;
   size_t new_sz = 0;
   const uint8_t *old_ptr = (const uint8_t *)lua_tolstring(L, 1, &old_sz);
   const uint8_t *new_ptr = (const uint8_t *)lua_tolstring(L, 2, &new_sz);

   if (old_sz <= 0 && new_sz <= 0)
   {
      lua_pushboolean(L, 0);
      lua_pushstring(L, "invalid params");
      return 2;
   }

   struct s_head h;
   memset(&h, 0, sizeof(h));

   struct bsdiff_stream stream;
   memset(&stream, 0, sizeof(stream));

   stream.malloc = malloc;
   stream.free = free;
   stream.opaque = &h;
   stream.write = _write_cb;

   {
      struct s_buf *b = _get_buf(&h);

      memcpy(b->buf, header_str, 16);
      b->len += 16;

      offtout(new_sz, &b->buf[b->len]);
      b->len += 8;
   }

   if (bsdiff(old_ptr, old_sz, new_ptr, new_sz, &stream))
   {
      _free_buf(&h);
      lua_pushboolean(L, 0);
      lua_pushstring(L, "failed to diff");
      return 2;
   }

   // collect output data length
   int out_sz = 0;
   struct s_buf *n = h.head;
   while (n)
   {
      out_sz += n->len;
      n = n->next;
   }

   if (out_sz > 0)
   {
      int len = 0;
      char *p = (char *)malloc(out_sz + 1);
      n = h.head;
      while (n)
      {
         memcpy(&p[len], n->buf, n->len);
         len += n->len;
         n = n->next;
      }

      lua_pushlstring(L, p, out_sz);
      free(p);
      _free_buf(&h);
      return 1;
   }
   else
   {
      _free_buf(&h);
      lua_pushboolean(L, 0);
      lua_pushstring(L, "failed to diff content");
      return 2;
   }
}

struct s_patch
{
   uint8_t *buf;
   int len;
   int offset;
};

static int
_read_cb(const struct bspatch_stream *stream, void *buffer, int size)
{
   struct s_patch *p = (struct s_patch *)stream->opaque;
   while (p->offset < p->len && size > 0)
   {
      int len = MIN_LEN((p->len - p->offset), size);
      memcpy(buffer, &p->buf[p->offset], len);
      size -= len;
      p->offset += len;
      printf("%d,%d,%d\n", (int)p->offset, (int)p->len, size);
   }
   if (size != 0) {
      printf(".. %d,%d,%d\n", (int)p->offset, (int)p->len, size);
   }
   return 0;
}

static int
_lpatch(lua_State *L)
{
   int types[2] = {LUA_TSTRING, LUA_TSTRING};
   if (!_check_type(L, types, 2))
   {
      lua_pushboolean(L, 0);
      lua_pushstring(L, "invalid params");
      return 2;
   }

   size_t old_sz = 0;
   size_t patch_sz = 0;
   uint8_t *old_ptr = (uint8_t *)lua_tolstring(L, 1, &old_sz);
   uint8_t *patch_ptr = (uint8_t *)lua_tolstring(L, 2, &patch_sz);

   if (old_sz <= 0 && patch_sz <= 0)
   {
      lua_pushboolean(L, 0);
      lua_pushstring(L, "invalid params");
      return 2;
   }

   if (memcmp(patch_ptr, header_str, 16) != 0)
   {
      lua_pushboolean(L, 0);
      lua_pushstring(L, "corrupt patch");
      return 2;
   }

   int64_t new_sz = offtin(patch_ptr + 16);
   if (new_sz <= 0)
   {
      lua_pushboolean(L, 0);
      lua_pushstring(L, "corrupt patch");
      return 2;
   }

   uint8_t *new_ptr = (uint8_t *)malloc(new_sz + 1);

   struct s_patch h;
   memset(&h, 0, sizeof(h));

   struct bspatch_stream stream;
   memset(&stream, 0, sizeof(stream));

   h.buf = patch_ptr;
   h.len = patch_sz;
   h.offset = 24;

   stream.opaque = &h;
   stream.read = _read_cb;

   if (bspatch(old_ptr, old_sz, new_ptr, new_sz, &stream))
   {
      lua_pushboolean(L, 0);
      lua_pushstring(L, "failed to patch");
      free(new_ptr);
      return 2;
   }

   lua_pushlstring(L, (const char *)new_ptr, new_sz);
   free(new_ptr);
   return 1;
}

static const luaL_Reg mnet_lib[] = {
    {"diff", _ldiff},
    {"patch", _lpatch},

    {NULL, NULL}};

LUALIB_API int
luaopen_bsdiff(lua_State *L)
{
   luaL_newlib(L, mnet_lib);
   return 1;
}
