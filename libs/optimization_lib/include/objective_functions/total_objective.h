//#pragma once
//
//
//#include <optimization-lib\objective-function.h>
//
//#include <memory>
//
//using namespace std;
//
//class TotalObjective : public ObjectiveFunction
//{
//public:
//	TotalObjective();
//	virtual void init();
//	virtual void updateX(const VectorXd& X);
//	virtual double value();
//	virtual void gradient(VectorXd& g);
//	virtual void hessian();
//
//	virtual void prepare_hessian();
//
//	// sub objectives
//	vector<unique_ptr<ObjectiveFunction>> objectiveList;
//};