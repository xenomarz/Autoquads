#pragma once
#ifndef OPTIMIZATION_LIB_EDGE_PAIR_ANGLE_OBJECTIVE_H
#define OPTIMIZATION_LIB_EDGE_PAIR_ANGLE_OBJECTIVE_H

// C includes
#define _USE_MATH_DEFINES
#include <math.h>

// Optimization lib includes
#include "../../utils/data_providers/edge_pair_data_provider.h"
#include "./edge_pair_objective.h"

template<Eigen::StorageOptions StorageOrder_>
class EdgePairAngleObjective : public EdgePairObjective<StorageOrder_>
{
public:
	/**
	 * Constructors and destructor
	 */
	EdgePairAngleObjective(const std::shared_ptr<MeshDataProvider>& mesh_data_provider, const std::shared_ptr<EdgePairDataProvider>& edge_pair_data_provider) :
		EdgePairObjective(mesh_data_provider, "Edge Pair Angle Objective", edge_pair_data_provider)
	{
		this->Initialize();
	}

	virtual ~EdgePairAngleObjective()
	{

	}

protected:
	/**
	 * Protected overrides
	 */
	void PreUpdate(const Eigen::VectorXd& x) override
	{
		auto& edge_pair_data_provider = this->GetEdgePairDataProvider();

		auto e1_v1_x_index = edge_pair_data_provider.GetEdge1Vertex1XIndex();
		auto e1_v1_y_index = edge_pair_data_provider.GetEdge1Vertex1YIndex();
		auto e1_v2_x_index = edge_pair_data_provider.GetEdge1Vertex2XIndex();
		auto e1_v2_y_index = edge_pair_data_provider.GetEdge1Vertex2YIndex();
		auto e2_v1_x_index = edge_pair_data_provider.GetEdge2Vertex1XIndex();
		auto e2_v1_y_index = edge_pair_data_provider.GetEdge2Vertex1YIndex();
		auto e2_v2_x_index = edge_pair_data_provider.GetEdge2Vertex2XIndex();
		auto e2_v2_y_index = edge_pair_data_provider.GetEdge2Vertex2YIndex();
		auto e1_y_diff = edge_pair_data_provider.GetEdge1YDiff();
		auto e1_x_diff = edge_pair_data_provider.GetEdge1XDiff();
		auto e2_y_diff = edge_pair_data_provider.GetEdge2YDiff();
		auto e2_x_diff = edge_pair_data_provider.GetEdge2XDiff();

		auto e1_quad_norm = edge_pair_data_provider.GetEdge1QuadrupledNrom();
		auto e2_quad_norm = edge_pair_data_provider.GetEdge2QuadrupledNrom();
	
		double e1_y_to_e1_squared_norm = e1_y_diff / edge_pair_data_provider.GetEdge1SquaredNrom();
		double e1_x_to_e1_squared_norm = e1_x_diff / edge_pair_data_provider.GetEdge1SquaredNrom();
		double e2_y_to_e2_squared_norm = e2_y_diff / edge_pair_data_provider.GetEdge2SquaredNrom();
		double e2_x_to_e2_squared_norm = e2_x_diff / edge_pair_data_provider.GetEdge2SquaredNrom();

		double e1_diff_prod_to_quad_norm = (2 * e1_x_diff * e1_y_diff) / e1_quad_norm;
		double e2_diff_prod_to_quad_norm = (2 * e2_x_diff * e2_y_diff) / e2_quad_norm;
		double e1_squares_diff_prod_to_quad_norm = (edge_pair_data_provider.GetEdge1XSquaredDiff() - edge_pair_data_provider.GetEdge1YSquaredDiff()) / e1_quad_norm;
		double e2_squares_diff_prod_to_quad_norm = (edge_pair_data_provider.GetEdge2XSquaredDiff() - edge_pair_data_provider.GetEdge2YSquaredDiff()) / e2_quad_norm;

		/**
		 * First partial derivatives
		 */
		this->sparse_index_to_first_derivative_value_map_[e1_v1_x_index] = e1_y_to_e1_squared_norm;
		this->sparse_index_to_first_derivative_value_map_[e1_v1_y_index] = e1_x_to_e1_squared_norm;
		this->sparse_index_to_first_derivative_value_map_[e1_v2_x_index] = e1_y_to_e1_squared_norm;
		this->sparse_index_to_first_derivative_value_map_[e1_v2_y_index] = e1_x_to_e1_squared_norm;
		this->sparse_index_to_first_derivative_value_map_[e2_v1_x_index] = e2_y_to_e2_squared_norm;
		this->sparse_index_to_first_derivative_value_map_[e2_v1_y_index] = e2_x_to_e2_squared_norm;
		this->sparse_index_to_first_derivative_value_map_[e2_v2_x_index] = e2_y_to_e2_squared_norm;
		this->sparse_index_to_first_derivative_value_map_[e2_v2_y_index] = e2_x_to_e2_squared_norm;

		/**
		 * Second partial derivatives
		 */
		this->sparse_indices_to_second_derivative_value_map_[{ e1_v1_x_index, e1_v1_x_index }] = e1_diff_prod_to_quad_norm;
		this->sparse_indices_to_second_derivative_value_map_[{ e1_v1_x_index, e1_v1_y_index }] = -e1_squares_diff_prod_to_quad_norm;
		this->sparse_indices_to_second_derivative_value_map_[{ e1_v1_x_index, e1_v2_x_index }] = -e1_diff_prod_to_quad_norm;
		this->sparse_indices_to_second_derivative_value_map_[{ e1_v1_x_index, e1_v2_y_index }] = e1_squares_diff_prod_to_quad_norm;
		this->sparse_indices_to_second_derivative_value_map_[{ e1_v1_x_index, e2_v1_x_index }] = 0;
		this->sparse_indices_to_second_derivative_value_map_[{ e1_v1_x_index, e2_v1_y_index }] = 0;
		this->sparse_indices_to_second_derivative_value_map_[{ e1_v1_x_index, e2_v2_x_index }] = 0;
		this->sparse_indices_to_second_derivative_value_map_[{ e1_v1_x_index, e2_v2_y_index }] = 0;

		this->sparse_indices_to_second_derivative_value_map_[{ e1_v2_x_index, e1_v1_x_index }] = e1_diff_prod_to_quad_norm;
		this->sparse_indices_to_second_derivative_value_map_[{ e1_v2_x_index, e1_v1_y_index }] = -e1_squares_diff_prod_to_quad_norm;
		this->sparse_indices_to_second_derivative_value_map_[{ e1_v2_x_index, e1_v2_x_index }] = -e1_diff_prod_to_quad_norm;
		this->sparse_indices_to_second_derivative_value_map_[{ e1_v2_x_index, e1_v2_y_index }] = e1_squares_diff_prod_to_quad_norm;
		this->sparse_indices_to_second_derivative_value_map_[{ e1_v2_x_index, e2_v1_x_index }] = 0;
		this->sparse_indices_to_second_derivative_value_map_[{ e1_v2_x_index, e2_v1_y_index }] = 0;
		this->sparse_indices_to_second_derivative_value_map_[{ e1_v2_x_index, e2_v2_x_index }] = 0;
		this->sparse_indices_to_second_derivative_value_map_[{ e1_v2_x_index, e2_v2_y_index }] = 0;

		this->sparse_indices_to_second_derivative_value_map_[{ e1_v1_y_index, e1_v1_x_index }] = e1_squares_diff_prod_to_quad_norm;
		this->sparse_indices_to_second_derivative_value_map_[{ e1_v1_y_index, e1_v1_y_index }] = e1_diff_prod_to_quad_norm;
		this->sparse_indices_to_second_derivative_value_map_[{ e1_v1_y_index, e1_v2_x_index }] = -e1_squares_diff_prod_to_quad_norm;
		this->sparse_indices_to_second_derivative_value_map_[{ e1_v1_y_index, e1_v2_y_index }] = -e1_diff_prod_to_quad_norm;
		this->sparse_indices_to_second_derivative_value_map_[{ e1_v1_y_index, e2_v1_x_index }] = 0;
		this->sparse_indices_to_second_derivative_value_map_[{ e1_v1_y_index, e2_v1_y_index }] = 0;
		this->sparse_indices_to_second_derivative_value_map_[{ e1_v1_y_index, e2_v2_x_index }] = 0;
		this->sparse_indices_to_second_derivative_value_map_[{ e1_v1_y_index, e2_v2_y_index }] = 0;

		this->sparse_indices_to_second_derivative_value_map_[{ e1_v2_y_index, e1_v1_x_index }] = e1_squares_diff_prod_to_quad_norm;
		this->sparse_indices_to_second_derivative_value_map_[{ e1_v2_y_index, e1_v1_y_index }] = e1_diff_prod_to_quad_norm;
		this->sparse_indices_to_second_derivative_value_map_[{ e1_v2_y_index, e1_v2_x_index }] = -e1_squares_diff_prod_to_quad_norm;
		this->sparse_indices_to_second_derivative_value_map_[{ e1_v2_y_index, e1_v2_y_index }] = -e1_diff_prod_to_quad_norm;
		this->sparse_indices_to_second_derivative_value_map_[{ e1_v2_y_index, e2_v1_x_index }] = 0;
		this->sparse_indices_to_second_derivative_value_map_[{ e1_v2_y_index, e2_v1_y_index }] = 0;
		this->sparse_indices_to_second_derivative_value_map_[{ e1_v2_y_index, e2_v2_x_index }] = 0;
		this->sparse_indices_to_second_derivative_value_map_[{ e1_v2_y_index, e2_v2_y_index }] = 0;

		this->sparse_indices_to_second_derivative_value_map_[{ e2_v1_x_index, e1_v1_x_index }] = 0;
		this->sparse_indices_to_second_derivative_value_map_[{ e2_v1_x_index, e1_v1_y_index }] = 0;
		this->sparse_indices_to_second_derivative_value_map_[{ e2_v1_x_index, e1_v2_x_index }] = 0;
		this->sparse_indices_to_second_derivative_value_map_[{ e2_v1_x_index, e1_v2_y_index }] = 0;
		this->sparse_indices_to_second_derivative_value_map_[{ e2_v1_x_index, e2_v1_x_index }] = e2_diff_prod_to_quad_norm;
		this->sparse_indices_to_second_derivative_value_map_[{ e2_v1_x_index, e2_v1_y_index }] = -e2_squares_diff_prod_to_quad_norm;
		this->sparse_indices_to_second_derivative_value_map_[{ e2_v1_x_index, e2_v2_x_index }] = -e2_diff_prod_to_quad_norm;
		this->sparse_indices_to_second_derivative_value_map_[{ e2_v1_x_index, e2_v2_y_index }] = e2_squares_diff_prod_to_quad_norm;

		this->sparse_indices_to_second_derivative_value_map_[{ e2_v2_x_index, e1_v1_x_index }] = 0;
		this->sparse_indices_to_second_derivative_value_map_[{ e2_v2_x_index, e1_v1_y_index }] = 0;
		this->sparse_indices_to_second_derivative_value_map_[{ e2_v2_x_index, e1_v2_x_index }] = 0;
		this->sparse_indices_to_second_derivative_value_map_[{ e2_v2_x_index, e1_v2_y_index }] = 0;
		this->sparse_indices_to_second_derivative_value_map_[{ e2_v2_x_index, e2_v1_x_index }] = e2_diff_prod_to_quad_norm;
		this->sparse_indices_to_second_derivative_value_map_[{ e2_v2_x_index, e2_v1_y_index }] = -e2_squares_diff_prod_to_quad_norm;
		this->sparse_indices_to_second_derivative_value_map_[{ e2_v2_x_index, e2_v2_x_index }] = -e2_diff_prod_to_quad_norm;
		this->sparse_indices_to_second_derivative_value_map_[{ e2_v2_x_index, e2_v2_y_index }] = e2_squares_diff_prod_to_quad_norm;

		this->sparse_indices_to_second_derivative_value_map_[{ e2_v1_y_index, e1_v1_x_index }] = 0;
		this->sparse_indices_to_second_derivative_value_map_[{ e2_v1_y_index, e1_v1_y_index }] = 0;
		this->sparse_indices_to_second_derivative_value_map_[{ e2_v1_y_index, e1_v2_x_index }] = 0;
		this->sparse_indices_to_second_derivative_value_map_[{ e2_v1_y_index, e1_v2_y_index }] = 0;
		this->sparse_indices_to_second_derivative_value_map_[{ e2_v1_y_index, e2_v1_x_index }] = e2_squares_diff_prod_to_quad_norm;
		this->sparse_indices_to_second_derivative_value_map_[{ e2_v1_y_index, e2_v1_y_index }] = e2_diff_prod_to_quad_norm;
		this->sparse_indices_to_second_derivative_value_map_[{ e2_v1_y_index, e2_v2_x_index }] = -e2_squares_diff_prod_to_quad_norm;
		this->sparse_indices_to_second_derivative_value_map_[{ e2_v1_y_index, e2_v2_y_index }] = -e2_diff_prod_to_quad_norm;

		this->sparse_indices_to_second_derivative_value_map_[{ e2_v2_y_index, e1_v1_x_index }] = 0;
		this->sparse_indices_to_second_derivative_value_map_[{ e2_v2_y_index, e1_v1_y_index }] = 0;
		this->sparse_indices_to_second_derivative_value_map_[{ e2_v2_y_index, e1_v2_x_index }] = 0;
		this->sparse_indices_to_second_derivative_value_map_[{ e2_v2_y_index, e1_v2_y_index }] = 0;
		this->sparse_indices_to_second_derivative_value_map_[{ e2_v2_y_index, e2_v1_x_index }] = e2_squares_diff_prod_to_quad_norm;
		this->sparse_indices_to_second_derivative_value_map_[{ e2_v2_y_index, e2_v1_y_index }] = e2_diff_prod_to_quad_norm;
		this->sparse_indices_to_second_derivative_value_map_[{ e2_v2_y_index, e2_v2_x_index }] = -e2_squares_diff_prod_to_quad_norm;
		this->sparse_indices_to_second_derivative_value_map_[{ e2_v2_y_index, e2_v2_y_index }] = -e2_diff_prod_to_quad_norm;
	}

private:
	/**
	 * Private overrides
	 */
	void CalculateValue(double& f) override
	{
		auto& edge_pair_data_provider = this->GetEdgePairDataProvider();
		f = std::atan2(edge_pair_data_provider.GetEdge1YDiff(), edge_pair_data_provider.GetEdge1XDiff()) - atan2(edge_pair_data_provider.GetEdge2YDiff(), edge_pair_data_provider.GetEdge2XDiff());
	}
};

#endif