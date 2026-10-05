CXX		=	g++
CXXFLAGS	=	-g -O3 -Wall

all:		predict predict_extra_credit

predict:	predict.cc trace.cc predictor.h branch.h trace.h my_predictor.h
		$(CXX) $(CXXFLAGS) -o predict predict.cc trace.cc

predict_extra_credit:        predict_extra_credit.cc trace.cc predictor.h branch.h trace.h my_predictor.h
		$(CXX) $(CXXFLAGS) -o predict_extra_credit predict_extra_credit.cc trace.cc


test_example:	test_example.cc predictor.h branch.h my_predictor.h
		$(CXX) $(CXXFLAGS) -o test_example test_example.cc

compare:	compare.cc trace.cc predictor.h branch.h trace.h my_predictor.h
		$(CXX) $(CXXFLAGS) -o compare compare.cc trace.cc

clean:
		rm -f predict predict_extra_credit test_example compare
