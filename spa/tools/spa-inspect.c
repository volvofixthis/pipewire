/* Pinos
 * Copyright (C) 2016 Wim Taymans <wim.taymans@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dlfcn.h>

#include <spa/node.h>
#include <spa/debug.h>


static void
inspect_port (SpaNode *node, SpaDirection direction, uint32_t port_id)
{
  SpaResult res;
  SpaFormat *format;
  void *state = NULL;
  SpaProps *props;

  while (true) {
    if ((res = spa_node_port_enum_formats (node, direction, port_id, &format, NULL, &state)) < 0) {
      if (res != SPA_RESULT_ENUM_END)
        printf ("got error %d\n", res);
      break;
    }
    if (format)
      spa_debug_format (format);
  }
  if ((res = spa_node_port_get_props (node, direction, port_id, &props)) < 0)
    printf ("port_get_props error: %d\n", res);
  else
    spa_debug_props (props, true);
}

static void
inspect_node (SpaNode *node)
{
  SpaResult res;
  unsigned int i, n_input, max_input, n_output, max_output;
  uint32_t *in_ports, *out_ports;
  SpaProps *props;

  if ((res = spa_node_get_props (node, &props)) < 0)
    printf ("can't get properties: %d\n", res);
  else
    spa_debug_props (props, true);

  if ((res = spa_node_get_n_ports (node, &n_input, &max_input, &n_output, &max_output)) < 0)
    printf ("can't get n_ports: %d\n", res);
  else
    printf ("supported ports %d %d %d %d\n", n_input, max_input, n_output, max_output);

  in_ports = alloca (n_input * sizeof (uint32_t));
  out_ports = alloca (n_input * sizeof (uint32_t));

  if ((res = spa_node_get_port_ids (node, n_input, in_ports, n_output, out_ports)) < 0)
    printf ("can't get port ids: %d\n", res);

  for (i = 0; i < n_input; i++) {
    printf (" input port: %08x\n", in_ports[i]);
    inspect_port (node, SPA_DIRECTION_INPUT, in_ports[i]);
  }
  for (i = 0; i < n_output; i++) {
    printf (" output port: %08x\n", out_ports[i]);
    inspect_port (node, SPA_DIRECTION_OUTPUT, out_ports[i]);
  }

}

static void
inspect_factory (const SpaHandleFactory *factory)
{
  SpaResult res;
  SpaHandle *handle;
  void *interface;
  void *state = NULL;

  printf ("factory name:\t\t'%s'\n", factory->name);
  printf ("factory info:\n");
  if (factory->info)
    spa_debug_dict (factory->info);
  else
    printf ("  none\n");

  handle = calloc (1, factory->size);
  if ((res = spa_handle_factory_init (factory, handle, NULL, NULL, 0)) < 0) {
    printf ("can't make factory instance: %d\n", res);
    return;
  }

  printf ("factory interfaces:\n");

  while (true) {
    const SpaInterfaceInfo *info;

    if ((res = spa_handle_factory_enum_interface_info (factory, &info, &state)) < 0) {
      if (res == SPA_RESULT_ENUM_END)
        break;
      else
        printf ("can't enumerate interfaces: %d\n", res);
    }
    printf (" interface: (%d) '%s' : '%s'\n", info->interface_id, info->name, info->description);

    if ((res = spa_handle_get_interface (handle, info->interface_id, &interface)) < 0) {
      printf ("can't get interface: %d\n", res);
      continue;
    }

    switch (info->interface_id) {
      case SPA_INTERFACE_ID_NODE:
        inspect_node (interface);
        break;
      default:
        printf ("skipping unknown interface\n");
        break;
    }
  }
}

int
main (int argc, char *argv[])
{
  SpaResult res;
  void *handle;
  SpaEnumHandleFactoryFunc enum_func;
  void *state = NULL;

  if (argc < 2) {
    printf ("usage: %s <plugin.so>\n", argv[0]);
    return -1;
  }

  if ((handle = dlopen (argv[1], RTLD_NOW)) == NULL) {
    printf ("can't load %s\n", argv[1]);
    return -1;
  }
  if ((enum_func = dlsym (handle, "spa_enum_handle_factory")) == NULL) {
    printf ("can't find function\n");
    return -1;
  }

  while (true) {
    const SpaHandleFactory *factory;

    if ((res = enum_func (&factory, &state)) < 0) {
      if (res != SPA_RESULT_ENUM_END)
        printf ("can't enumerate factories: %d\n", res);
      break;
    }
    inspect_factory (factory);
  }

  return 0;
}
