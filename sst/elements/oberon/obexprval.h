
#ifndef SST_OBERON_EXPR_VALUE_BASE
#define SST_OBERON_EXPR_VALUE_BASE

class OberonExpressionValue {

	public:
		virtual int64_t getAsInt64() = 0;
		virtual double  getAsFP64()  = 0;

}

#endif
