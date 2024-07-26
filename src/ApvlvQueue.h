/*
 * This file is part of the apvlv package
 *
 * Copyright (C) 2008 Alf.
 *
 * Contact: Alf <naihe2010@126.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2.0 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */
/* @CPPFILE ApvlvCmds.h
 *
 *  Author: Alf <naihe2010@126.com>
 */

#ifndef _APVLV_QUEUE_H_
#define _APVLV_QUEUE_H_

#include <mutex>
#include <queue>

namespace apvlv
{

using namespace std;

template <class T> class LockQueue
{
public:
  LockQueue () = default;
  LockQueue (const LockQueue &) = delete;
  LockQueue &operator= (const LockQueue &) = delete;
  ~LockQueue () = default;

  void
  push (const T &node)
  {
    lock_guard<mutex> lock (mMutex);
    mQueueInternal.push (node);
  }

  void
  push (T &&node)
  {
    lock_guard<mutex> lock (mMutex);
    mQueueInternal.push (move (node));
  }

  bool
  pop (T &node)
  {
    lock_guard<mutex> lock (mMutex);
    if (mQueueInternal.empty ())
      return false;

    node = move (mQueueInternal.front ());
    mQueueInternal.pop ();
    return true;
  }

  void
  empty ()
  {
    lock_guard<mutex> lock (mMutex);
    return mQueueInternal.empty ();
  }

  void
  clear ()
  {
    lock_guard<mutex> lock (mMutex);
    mQueueInternal = {};
  }

private:
  queue<T> mQueueInternal;
  mutex mMutex;
};

}

#endif
