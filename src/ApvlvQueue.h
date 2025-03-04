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

#include <condition_variable>
#include <mutex>
#include <queue>

namespace apvlv
{

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
    std::lock_guard<std::mutex> lock (mMutex);
    mQueueInternal.push (node);
  }

  void
  push (T &&node)
  {
    std::lock_guard<std::mutex> lock (mMutex);
    mQueueInternal.push (std::move (node));
  }

  bool
  pop (T &node)
  {
    std::lock_guard<std::mutex> lock (mMutex);
    if (mQueueInternal.empty ())
      return false;

    node = std::move (mQueueInternal.front ());
    mQueueInternal.pop ();
    return true;
  }

  void
  empty ()
  {
    std::lock_guard<std::mutex> lock (mMutex);
    return mQueueInternal.empty ();
  }

  void
  clear ()
  {
    std::lock_guard<std::mutex> lock (mMutex);
    mQueueInternal = {};
  }

private:
  std::queue<T> mQueueInternal;
  std::mutex mMutex;
};

class Token;
class TokenDispatcher final
{
public:
  TokenDispatcher (int count, bool enable)
      : mCount (count), mEnableSpecial (enable)
  {
    mDispatchedCount = 0;
  }
  ~TokenDispatcher () = default;

  std::unique_ptr<Token> getToken (bool isSpecial);

  void returnToken (Token *token);

  TokenDispatcher (const TokenDispatcher &) = delete;
  TokenDispatcher &operator= (const TokenDispatcher &) = delete;
  TokenDispatcher (TokenDispatcher &&) = delete;
  TokenDispatcher &operator= (TokenDispatcher &&) = delete;

private:
  int mCount;
  bool mEnableSpecial;
  int mDispatchedCount;
  std::mutex mMutex;
  std::condition_variable mCondition;
};

class Token final
{
public:
  explicit Token (TokenDispatcher *parent)
      : mParent (parent), mIsReturned (false)
  {
  }
  ~Token ()
  {
    if (!mIsReturned)
      mParent->returnToken (this);
  }

private:
  TokenDispatcher *mParent;
  bool mIsReturned;
};

}

#endif
