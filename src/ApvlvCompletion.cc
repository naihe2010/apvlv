/*
 * This file is part of the apvlv package
 * Copyright (C) <2008> Alf
 *
 * Contact: Alf <naihe2010@126.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */
/* @CPPFILE ApvlvCompletion.cc
 *
 *  Author: Alf <naihe2010@126.com>
 */

#include "ApvlvCompletion.h"

#include <cstring>

namespace apvlv
{
    ApvlvCompletion::ApvlvCompletion ()
    {
#ifdef APVLV_NO_G_COMP
      mArray = g_ptr_array_new ();
      mCache = nullptr;
#else
      mComp = g_completion_new (nullptr);
#endif
    }

    ApvlvCompletion::~ApvlvCompletion ()
    {
#ifdef APVLV_NO_G_COMP
      g_ptr_array_free (mArray, TRUE);
      if (mCache)
        {
          g_list_free (mCache);
          mCache = nullptr;
        }
#else
      g_completion_free (mComp);
#endif
    }

    GList *
    ApvlvCompletion::complete (const gchar *prefix, gchar **context)
    {
#ifdef APVLV_NO_G_COMP
      size_t i, count;

      if (mCache)
        {
          g_list_free (mCache);
          mCache = nullptr;
        }

      for (count = 0, i = 0; i < mArray->len; ++i)
        {
          gchar *str;

          str = static_cast <gchar *> (g_ptr_array_index (mArray, i));
          if (memcmp (prefix, str, strlen (prefix)) == 0)
            {
              mCache = g_list_append (mCache, str);
              ++count;
            }
        }

      if (count == 1)
        {
          if (context)
            {
              *context = static_cast <gchar *> (mCache->data);
            }
        }

      return mCache;
#else
      return g_completion_complete (mComp, prefix, context);
#endif
    }

    gboolean
    ApvlvCompletion::add_items (GList *items)
    {
#ifdef APVLV_NO_G_COMP
      GList *list;

      for (list = items; list; list = g_list_next (list))
        {
          auto *str = static_cast <gchar *> (list->data);
          g_ptr_array_add (mArray, str);
        }
#else
      g_completion_add_items (mComp, items);
#endif

      return TRUE;
    }
}

// Local Variables:
// mode: c++
// End:
