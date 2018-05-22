// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <Python.h>

#include <vector>
#include <string>
#include <numeric>

#include <cstddef>

#include <sst/core/serialization/serializer.h>


#if 0
#include <boost/archive/basic_archive.hpp>
#include <boost/archive/detail/common_oarchive.hpp>
#include <boost/archive/detail/polymorphic_oarchive_route.hpp>
#endif

#include "pymodule.h"
#include "pyproto.h"

namespace SST {
namespace PyProtoNS {

#if 0
class PyEvent_oarchive : public boost::archive::detail::common_oarchive<PyEvent_oarchive>
{
    // permit serialization system privileged access to permit
    // implementation of inline templates for maximum speed.
    friend class boost::archive::detail::interface_oarchive<PyEvent_oarchive>;
    friend class boost::archive::detail::polymorphic_oarchive_route<PyEvent_oarchive>;
    friend class boost::archive::save_access;

    // member template for saving primitive types.
    // Specialize for any types/templates that require special treatment
    void save(const boost::archive::class_id_type & t) { }
    void save(const boost::archive::class_id_optional_type & t) { }
    void save(const boost::archive::tracking_type & t) { }
    void save(const boost::archive::object_id_type & t) { }
    void save(const boost::archive::version_type & t) { }
    void save(const boost::archive::class_name_type & t)
    {
        //fprintf(stderr, "save of class_name_type called: %s\n", t.t);
        if ( ! set_BaseName ) {
            PyObject_SetAttrString((PyObject*)pyEvent, "type", PyString_FromString(t.t));
        }
        set_BaseName = true;
    }

    void setItem(PyObject *obj)
    {
        fprintf(stderr, "Setting %s\n", current_name.c_str());
        PyDict_SetItemString(pyEvent->dict, current_name.c_str(), obj);
    }

    void save(const int & t)
    {
        setItem(PyInt_FromLong(t));
    }

    void save(const long & t)
    {
        setItem(PyLong_FromLong(t));
    }

    void save(const unsigned long & t)
    {
        setItem(PyLong_FromUnsignedLong(t));
    }

    void save(const long long & t)
    {
        setItem(PyLong_FromLongLong(t));
    }

    void save(const unsigned long long & t)
    {
        setItem(PyLong_FromUnsignedLongLong(t));
    }

    void save(const float & t)
    {
        setItem(PyFloat_FromDouble(t));
    }

    void save(const double & t)
    {
        setItem(PyFloat_FromDouble(t));
    }

    void save(const bool& t)
    {
        setItem(PyBool_FromLong(t));
    }

    void save(const std::string & t)
    {
        setItem(PyString_FromString(t.c_str()));
    }

    template<class T>
    void save(T & t)
    {
        fprintf(stderr, "Type [%s] (%s) needs an implementation!\n",
                current_name.c_str(), typeid(T).name());
    }

    void buildCurrentName()
    {
        current_name = std::accumulate( stack.begin(), stack.end(), std::string(""),
                    [](const std::string &a, const std::string &b) {
                        return a.empty() ? b : (a + "::" + b);
                    });
    }


    PyEvent_t *pyEvent;
    std::vector<std::string> stack;
    std::string current_name;
    bool set_BaseName;

public:
    template <class _Elem, class _Tr>
    PyEvent_oarchive( std::basic_ostream<_Elem, _Tr> & os, unsigned int flags) : boost::archive::detail::common_oarchive<PyEvent_oarchive>(flags)
    {
        PyObject *args = Py_BuildValue("()");
        pyEvent = (PyEvent_t*)PyObject_CallObject((PyObject*)getEventObject(), args);
        Py_XDECREF(args);

        set_BaseName = false;
    }

    virtual ~PyEvent_oarchive()
    {
        Py_XDECREF((PyObject*)pyEvent);
    }

    /** Returns new reference */
    PyEvent_t* getEvent()
    {
        Py_XINCREF((PyObject*)pyEvent);
        return pyEvent;
    }


    // archives are expected to support this function
    void save_binary(const void *address, std::size_t count)
    {
        fprintf(stderr, "save_binary of %p[%zu] called\n", address, count);
    }

    // Called before a save()
    void save_start(char const *name)
    {
        //fprintf(stderr, "save_start(%s)\n", name);
        if ( name )
            stack.push_back(name);
        buildCurrentName();
    }

    void save_end(char const *name)
    {
        //fprintf(stderr, "save_end(%s)\n", name);
        if ( name && !stack.empty() )
            stack.pop_back();
        buildCurrentName();
    }
};



typedef boost::archive::detail::polymorphic_oarchive_route<PyEvent_oarchive> polymorphic_PyEvent_oarchive;

#endif
}
}

#if 0
BOOST_SERIALIZATION_REGISTER_ARCHIVE(
    SST::PyProtoNS::polymorphic_PyEvent_oarchive
)

#endif
