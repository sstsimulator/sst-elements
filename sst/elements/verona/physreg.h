
#ifndef _physreg_H
#define _physreg_H

#include <cstdint>
#include <cstring>
#include <cassert>
#include <cstdio>
#include <iostream>
#include <cmath>

#define ALIGNED(a, b) ((a % b) == 0)

class VeronaRegisterGroup {
public:

  VeronaRegisterGroup(unsigned int reg_count,
	unsigned int reg_width_bytes, 
	bool reg_zero_is_zero, string group_name);
  void* GetRegisterContents(unsigned int reg_num);
  void PrintRegisterGroup();

private:
  VeronaRegisterGroup();  // for serialization only
  VeronaRegisterGroup(const VeronaRegisterGroup&); // do not implement
  void operator=(const VeronaRegisterGroup&); // do not implement

  void** register_group;
  bool zero_reg_is_zero;
  unsigned int register_count;
  unsigned int register_width_bytes;
  string reg_group_name;
  
  void internal_copy(void* source, void* dest, int length);

  friend class boost::serialization::access;
  template<class Archive>
  void save(Archive & ar, const unsigned int version) const
  {
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Component);
  }

  template<class Archive>
  void load(Archive & ar, const unsigned int version) 
  {
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Component);
  }
  BOOST_SERIALIZATION_SPLIT_MEMBER()
 
};

#endif /* _physreg_H */