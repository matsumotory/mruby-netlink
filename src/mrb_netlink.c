/*
 * ** mrb_netlink - netlink class for mruby
 * **
 * ** Copyright (c) mod_mruby developers 2012-
 * **
 * ** Permission is hereby granted, free of charge, to any person obtaining
 * ** a copy of this software and associated documentation files (the
 * ** "Software"), to deal in the Software without restriction, including
 * ** without limitation the rights to use, copy, modify, merge, publish,
 * ** distribute, sublicense, and/or sell copies of the Software, and to
 * ** permit persons to whom the Software is furnished to do so, subject to
 * ** the following conditions:
 * **
 * ** The above copyright notice and this permission notice shall be
 * ** included in all copies or substantial portions of the Software.
 * **
 * ** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * ** EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * ** MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * ** IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * ** CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * ** TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * ** SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * **
 * ** [ MIT license: http://www.opensource.org/licenses/mit-license.php ]
 * */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <net/if.h>
#include <asm/types.h>
#include <libnetlink.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>

#include "mruby.h"
#include "mruby/variable.h"
#include "mruby/data.h"

struct iplink_req {
  struct nlmsghdr n;
  struct ifinfomsg i;
  char buf[1024];
};

typedef struct {
  struct rtnl_handle *rth;
  struct iplink_req *req;
} mrb_netlink_context;

static void mrb_netlink_context_free(mrb_state *mrb, void *p)
{
}

static const struct mrb_data_type mrb_netlink_context_type = {
  "mrb_netlink_context", mrb_netlink_context_free,
};

static mrb_netlink_context *mrb_netlink_get_context(mrb_state *mrb,  mrb_value self, char *ctx_flag)
{
  mrb_netlink_context *c;
  mrb_value context;

  context = mrb_iv_get(mrb, self, mrb_intern_cstr(mrb, ctx_flag));
  Data_Get_Struct(mrb, context, &mrb_netlink_context_type, c);
  if (!c)
    mrb_raise(mrb, E_RUNTIME_ERROR, "get mrb_netlink_context failed");

  return c;
}

static mrb_value mrb_netlink_init(mrb_state *mrb, mrb_value self)
{
  char *dev;
  mrb_netlink_context *nctx = (mrb_netlink_context *)mrb_malloc(mrb, sizeof(mrb_netlink_context));

  nctx->rth = (struct rtnl_handle *)mrb_malloc(mrb, sizeof(struct rtnl_handle));
  nctx->req = (struct iplink_req *)mrb_malloc(mrb, sizeof(struct iplink_req));

  nctx->rth->fd = -1;
  if (rtnl_open(nctx->rth, 0) < 0)
    mrb_raise(mrb, E_RUNTIME_ERROR, "Cannot open rtnetlink");

  nctx->req->n.nlmsg_len   = NLMSG_LENGTH(sizeof(struct ifinfomsg));
  nctx->req->n.nlmsg_flags = NLM_F_REQUEST;
  nctx->req->n.nlmsg_type  = RTM_NEWLINK;
  nctx->req->i.ifi_family  = AF_UNSPEC;

  mrb_get_args(mrb, "z", &dev);
  nctx->req->i.ifi_index = if_nametoindex(dev);
  if (nctx->req->i.ifi_index == 0)
    mrb_raisef(mrb, E_RUNTIME_ERROR, "Cannot find device \"%S\"", mrb_str_new_cstr(mrb, dev));

  mrb_iv_set(mrb
    , self
    , mrb_intern_lit(mrb, "mrb_netlink_context")
    , mrb_obj_value(Data_Wrap_Struct(mrb
      , mrb->object_class
      , &mrb_netlink_context_type
      , (void *)nctx)
    )
  );

  return self;
}

static mrb_value mrb_netlink_up(mrb_state *mrb, mrb_value self)
{
  int ret;
  mrb_netlink_context *nctx = mrb_netlink_get_context(mrb, self, "mrb_netlink_context");

  nctx->req->i.ifi_change |= IFF_UP;
  nctx->req->i.ifi_flags |= IFF_UP;
  ret = rtnl_talk(nctx->rth, &nctx->req->n, 0, 0, NULL, NULL, NULL);

  mrb_iv_set(mrb
    , self
    , mrb_intern_lit(mrb, "mrb_netlink_context")
    , mrb_obj_value(Data_Wrap_Struct(mrb
      , mrb->object_class
      , &mrb_netlink_context_type
      , (void *)nctx)
    )
  );

  return mrb_fixnum_value(ret);
}

static mrb_value mrb_netlink_down(mrb_state *mrb, mrb_value self)
{
  int ret;
  mrb_netlink_context *nctx = mrb_netlink_get_context(mrb, self, "mrb_netlink_context");

  nctx->req->i.ifi_change |= IFF_UP;
  nctx->req->i.ifi_flags &= ~IFF_UP;
  ret = rtnl_talk(nctx->rth, &nctx->req->n, 0, 0, NULL, NULL, NULL);

  mrb_iv_set(mrb
    , self
    , mrb_intern_lit(mrb, "mrb_netlink_context")
    , mrb_obj_value(Data_Wrap_Struct(mrb
      , mrb->object_class
      , &mrb_netlink_context_type
      , (void *)nctx)
    )
  );

  return mrb_fixnum_value(ret);
}

static mrb_value mrb_netlink_talk(mrb_state *mrb, mrb_value self)
{
  mrb_netlink_context *nctx = mrb_netlink_get_context(mrb, self, "mrb_netlink_context");
  //rtnl_talk(nctx->rth, &nctx->req->n, 0, 0, NULL, NULL, NULL);
  rtnl_talk(nctx->rth, &nctx->req->n, 0, 0, NULL, NULL, NULL);
  return self;
}

static mrb_value mrb_netlink_close(mrb_state *mrb, mrb_value self)
{
  mrb_netlink_context *nctx = mrb_netlink_get_context(mrb, self, "mrb_netlink_context");
  rtnl_close(nctx->rth);
  return self;
}

void mrb_mruby_netlink_gem_init(mrb_state *mrb)
{ 
  struct RClass *netlink;
  netlink = mrb_define_class(mrb, "Netlink", mrb->object_class);
  mrb_define_method(mrb, netlink, "initialize", mrb_netlink_init, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, netlink, "up", mrb_netlink_up, MRB_ARGS_NONE());
  mrb_define_method(mrb, netlink, "down", mrb_netlink_down, MRB_ARGS_NONE());
  //mrb_define_method(mrb, netlink, "set", mrb_netlink_set, MRB_ARGS_ANY());
  mrb_define_method(mrb, netlink, "talk", mrb_netlink_talk, MRB_ARGS_NONE());
  mrb_define_method(mrb, netlink, "close", mrb_netlink_close, MRB_ARGS_NONE());
}

void mrb_mruby_netlink_gem_final(mrb_state *mrb)
{
}

