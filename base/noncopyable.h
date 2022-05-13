// @Author Z
// @Email 3161349290@qq.com
#pragma once

class noncopyable
{
protected:
  noncopyable() {}
  ~noncopyable() {}

private:
  noncopyable(const noncopyable &);
  const noncopyable &operator=(const noncopyable &);
};