#pragma once
#ifndef OPTIMIZATION_LIB_OBJECTIVE_FUNCTION_H
#define OPTIMIZATION_LIB_OBJECTIVE_FUNCTION_H

// STL Includes
#include <vector>
#include <memory>
#include <atomic>
#include <functional>
#include <type_traits>
#include <string>
#include <mutex>

// Eigen Includes
#include <Eigen/Core>

// Optimization Lib Includes
#include "../utils/objective_function_data_provider.h"
#include "../utils/utils.h"
#include <Eigen/src/Core/util/ForwardDeclarations.h>
#include <Eigen/src/Core/util/ForwardDeclarations.h>

class ObjectiveFunction
{
public:
	/**
	 * Public type definitions
	 */
	enum class UpdateOptions : uint32_t
	{
		NONE = 0,
		VALUE = 1,
		GRADIENT = 2,
		HESSIAN = 4,
		ALL = 7
	};

	/**
	 * Constructor and destructor
	 */
	ObjectiveFunction(const std::shared_ptr<ObjectiveFunctionDataProvider>& objective_function_data_provider, const std::string& name);
	virtual ~ObjectiveFunction();

	/**
	 * Getters
	 */
	inline double GetValue() const
	{
		std::lock_guard<std::mutex> lock(m_);
		return f_;
	}

	inline const Eigen::VectorXd& GetValuePerVertex() const
	{
		std::lock_guard<std::mutex> lock(m_);
		return f_per_vertex_;
	}

	inline const Eigen::VectorXd& GetGradient() const
	{
		std::lock_guard<std::mutex> lock(m_);
		return g_;
	}

	inline const std::vector<int>& GetII() const
	{
		std::lock_guard<std::mutex> lock(m_);
		return ii_;
	}

	inline const std::vector<int>& GetJJ() const
	{
		std::lock_guard<std::mutex> lock(m_);
		return jj_;
	}

	inline const std::vector<double>& GetSS() const
	{
		std::lock_guard<std::mutex> lock(m_);
		return ss_;
	}

	inline const Eigen::SparseMatrix<double, Eigen::ColMajor>& GetHessianColMajor() const
	{
		std::lock_guard<std::mutex> lock(m_);
		return H_cm_;
	}

	inline const Eigen::SparseMatrix<double, Eigen::RowMajor>& GetHessianRowMajor() const
	{
		std::lock_guard<std::mutex> lock(m_);
		return H_rm_;
	}

	inline double GetWeight() const
	{
		std::lock_guard<std::mutex> lock(m_);
		return w_;
	}

	/**
	 * Setters
	 */
	void SetWeight(const double w);
	const std::string GetName() const;

	/**
	 * Public methods
	 */

	// Initializes the objective function object. Must be called from any derived class constructor.
	void Initialize();

	// Update value, gradient and hessian for a given x
	void Update(const Eigen::VectorXd& x, const UpdateOptions update_options = UpdateOptions::ALL);

protected:

	/**
	 * Protected methods
	 */
	virtual void PreInitialize();
	virtual void PostInitialize();
	virtual void PreUpdate(const Eigen::VectorXd& x);
	virtual void PostUpdate(const Eigen::VectorXd& x);

	/**
	 * Protected Fields
	 */

	// Objective function data provider
	std::shared_ptr<ObjectiveFunctionDataProvider> objective_function_data_provider_;

	// Mutex
	mutable std::mutex m_;

	// Elements count
	Eigen::DenseIndex domain_faces_count_;
	Eigen::DenseIndex domain_vertices_count_;
	Eigen::DenseIndex image_faces_count_;
	Eigen::DenseIndex image_vertices_count_;
	Eigen::DenseIndex variables_count_;

private:

	/**
	 * Private methods
	 */

	// Gradient and hessian initializers
	virtual void InitializeValue(double& f, Eigen::VectorXd& f_per_vertex);
	virtual void InitializeGradient(Eigen::VectorXd& g);
	virtual void InitializeHessian(std::vector<int>& ii, std::vector<int>& jj, std::vector<double>& ss) = 0;

	// Value, gradient and hessian calculation functions
	// TODO: Remove input argument 'const Eigen::VectorXd& x' from value, gradient and hessian calculation. This argument should be processed in PreUpdate().
	virtual void CalculateValue(double& f, Eigen::VectorXd& f_per_vertex) = 0;
	virtual void CalculateGradient(Eigen::VectorXd& g) = 0;
	virtual void CalculateHessian(std::vector<double>& ss) = 0;

	/**
	 * Private fields
	 */

	// Value
	double f_;
	Eigen::VectorXd f_per_vertex_;

	// Gradient
	Eigen::VectorXd g_;

	// Hessian (sparse representation)
	// TODO: remove ii_, jj_ and ss_ and use vector of triplets
	std::vector<int> ii_; 
	std::vector<int> jj_;
	std::vector<double> ss_;
	Eigen::SparseMatrix<double, Eigen::ColMajor> H_cm_;
	Eigen::SparseMatrix<double, Eigen::RowMajor> H_rm_;

	// Weight
	std::atomic<double> w_;

	// Name
	const std::string name_;
};

// http://blog.bitwigglers.org/using-enum-classes-as-type-safe-bitmasks/
inline ObjectiveFunction::UpdateOptions operator | (const ObjectiveFunction::UpdateOptions lhs, const ObjectiveFunction::UpdateOptions rhs)
{
	using T = std::underlying_type_t <ObjectiveFunction::UpdateOptions>;
	return static_cast<ObjectiveFunction::UpdateOptions>(static_cast<T>(lhs) | static_cast<T>(rhs));
}

inline ObjectiveFunction::UpdateOptions& operator |= (ObjectiveFunction::UpdateOptions& lhs, ObjectiveFunction::UpdateOptions rhs)
{
	lhs = lhs | rhs;
	return lhs;
}

inline ObjectiveFunction::UpdateOptions operator & (const ObjectiveFunction::UpdateOptions lhs, const ObjectiveFunction::UpdateOptions rhs)
{
	using T = std::underlying_type_t <ObjectiveFunction::UpdateOptions>;
	return static_cast<ObjectiveFunction::UpdateOptions>(static_cast<T>(lhs) & static_cast<T>(rhs));
}

inline ObjectiveFunction::UpdateOptions& operator &= (ObjectiveFunction::UpdateOptions& lhs, ObjectiveFunction::UpdateOptions rhs)
{
	lhs = lhs & rhs;
	return lhs;
}

#endif