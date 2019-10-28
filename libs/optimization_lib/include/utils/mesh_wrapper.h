#pragma once
#ifndef OPTIMIZATION_LIB_MESH_WRAPPER_H
#define OPTIMIZATION_LIB_MESH_WRAPPER_H

// STL Includes
#include <vector>
#include <map>
#include <unordered_map>
#include <tuple>
#include <utility>
#include <algorithm>
#include <functional>
#include <string>

// Boost includes
#include <boost/signals2/signal.hpp>
#include <boost/functional/hash.hpp>

// Eigen Includes
#include <Eigen/Core>
#include <Eigen/Sparse>

// Optimization lib includes
#include "./objective_function_data_provider.h"
#include "../utils/utils.h"

class MeshWrapper : public ObjectiveFunctionDataProvider
{
public:

	/**
	 * Public type definitions
	 */
	enum class SoupInitType {
		RANDOM,
		ISOMETRIC
	};

	using ModelLoadedCallback = void();

	using EV2EVMap = std::vector<std::pair<std::pair<int64_t, int64_t>, std::pair<int64_t, int64_t>>>;

	/**
	 * Constructors and destructor
	 */
	MeshWrapper();
	MeshWrapper(const Eigen::MatrixX3d& v, const Eigen::MatrixX3i& f);
	MeshWrapper(const std::string& modelFilename);
	virtual ~MeshWrapper();

	/**
	 * Setters
	 */
	void SetImageVertices(const Eigen::MatrixX2d& v_im);

	/**
	 * Getters
	 */
	const Eigen::MatrixX3i& GetImageFaces() const;
	const Eigen::MatrixX2d& GetImageVertices() const;
	const Eigen::MatrixX2i& GetImageEdges() const;

	/**
	 * Overrides
	 */
	const Eigen::MatrixX3i& GetDomainFaces() const override;
	const Eigen::MatrixX3d& GetDomainVertices() const override;
	const Eigen::MatrixX2i& GetDomainEdges() const override;
	const Eigen::MatrixX3d& GetD1() const override;
	const Eigen::MatrixX3d& GetD2() const override;
	const Eigen::SparseMatrix<double>& GetCorrespondingVertexPairsCoefficients() const override;
	const Eigen::VectorXd& GetCorrespondingVertexPairsEdgeLength() const override;
	int64_t GetImageVerticesCount() const override;
	const EV2EVMap& GetCorrespondingEdgeVertices() const;

	/**
	 * Public methods
	 */
	Eigen::VectorXi GetImageFaceVerticesIndices(int64_t face_index);
	Eigen::MatrixXd GetImageVertices(const Eigen::VectorXi& vertex_indices);
	void LoadModel(const std::string& model_file_path);
	void RegisterModelLoadedCallback(const std::function<ModelLoadedCallback>& model_loaded_callback);

private:
	/**
	 * Private type definitions
	 */
	using EdgeDescriptor = std::pair<int64_t, int64_t>;
	using ED2EIMap = std::unordered_map<EdgeDescriptor, int64_t, Utils::PairHash, Utils::PairEquals>;
	using VI2VIsMap = std::unordered_map<int64_t, std::vector<int64_t>>;
	using VI2VIMap = std::unordered_map<int64_t, int64_t>;
	using EI2EIsMap = std::unordered_map<int64_t, std::vector<int64_t>>;
	using EI2EIMap = std::unordered_map<int64_t, int64_t>;

	/**
	 * Private functions
	 */
	void Initialize();
	
	/**
	* Private enums
	*/
	enum class ModelFileType
	{
		OBJ,
		OFF,
		UNKNOWN
	};

	/**
	 * General use mesh methods
	 */
	void ComputeEdges(const Eigen::MatrixX3i& f, Eigen::MatrixX2i& e);
	void NormalizeVertices(Eigen::MatrixX3d& v);

	/**
	 * Discrete operators
	 */
	void ComputeSurfaceGradientPerFace(const Eigen::MatrixX3d& v, const Eigen::MatrixX3i& f, Eigen::MatrixX3d& d1, Eigen::MatrixX3d& d2);

	/**
	 * Triangle soup methods
	 */

	// Soup generation
	void GenerateSoupFaces(const Eigen::MatrixX3i& f_in, Eigen::MatrixX3i& f_out);
	void FixFlippedFaces(const Eigen::MatrixX3i& f_im, Eigen::MatrixX2d& v_im);
	void GenerateRandom2DSoup(const Eigen::MatrixX3i& f_in, Eigen::MatrixX3i& f_out, Eigen::MatrixX2d& v_out);

	// Edge descriptor -> edge index map
	void ComputeEdgeDescriptorMap(const Eigen::MatrixX2i& e, ED2EIMap& ed_2_ei);

	// Domain edge index <-> image edge index maps
	void ComputeEdgeIndexMaps();

	// Domain vertex index <-> image vertex index maps
	void ComputeVertexIndexMaps();

	// Image vertices corresponding pairs and image edges corresponding pairs
	void ComputeCorrespondingPairs();
	void ComputeCorrespondingVertexPairsCoefficients();
	void ComputeCorrespondingVertexPairsEdgeLength();

	/**
	 * Private methods
	 */
	ModelFileType GetModelFileType(const std::string& modelFilePath);

	/**
	 * Fields
	 */

	// Domain matrices
	Eigen::MatrixX3d v_dom_;
	Eigen::MatrixX3i f_dom_;
	Eigen::MatrixX2i e_dom_;

	// Image matrices
	Eigen::MatrixX2d v_im_;
	Eigen::MatrixX3i f_im_;
	Eigen::MatrixX2i e_im_;

	// Discrete partial-derivatives matrices
	Eigen::MatrixX3d d1_;
	Eigen::MatrixX3d d2_;

	// Image corresponding pairs
	std::vector<std::pair<int64_t, int64_t>> cv_pairs_;
	std::vector<std::pair<int64_t, int64_t>> ce_pairs_;
	EV2EVMap cev_pairs_;
	Eigen::SparseMatrix<double> cv_pairs_coefficients_;
	Eigen::VectorXd cv_pairs_edge_length_;

	// Maps
	ED2EIMap ed_im_2_ei_im_;
	ED2EIMap ed_dom_2_ei_dom_;
	VI2VIsMap v_dom_2_v_im_;
	VI2VIMap v_im_2_v_dom_;
	EI2EIsMap e_dom_2_e_im_;
	EI2EIMap e_im_2_e_dom_;

	// Boost signals
	boost::signals2::signal<ModelLoadedCallback> model_loaded_signal_;
};

#endif