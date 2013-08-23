
#ifndef _H_SST_OBERON_FP64_VAL
#define _H_SST_OBERON_FP64_VAL

namespace SST {
namespace Oberon {

class OberonFP64ExprValue : public OberonExpressionValue {

        private:
                double value;

        public:
               	OberonFP64ExprValue(double v);
                int64_t getAsInt64();
                double  getAsFP64();

};

}
}

#endif
