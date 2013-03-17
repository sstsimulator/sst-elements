
#ifndef _physreg_H
#define _physreg_H

#include <cstdint>

#define ALIGNED(a, b) ((a % b) == 0)

class VeronaRegisterGroup {
public:

  VeronaRegisterGroup(int reg_count, int reg_width_bytes, bool reg_zero_is_zero);

private:
  VeronaRegisterGroup();  // for serialization only
  VeronaRegisterGroup(const VeronaRegisterGroup&); // do not implement
  void operator=(const VeronaRegisterGroup&); // do not implement

  void** register_group;
  bool zero_reg_is_zero;
  int register_count;
  int register_width_bytes;
  
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