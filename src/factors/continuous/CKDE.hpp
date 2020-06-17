#ifndef PGM_DATASET_CKDE_HPP
#define PGM_DATASET_CKDE_HPP

#include <iostream>

#include <CL/cl2.hpp>
#include <opencl/opencl_config.hpp>
#include <pybind11/pybind11.h>
#include <Eigen/Dense>
#include <pybind11/eigen.h>
#include <dataset/dataset.hpp>
#include <util/math_constants.hpp>

namespace py = pybind11;
using dataset::DataFrame;
using Eigen::VectorXd, Eigen::Ref, Eigen::LLT;
using opencl::OpenCLConfig, opencl::OpenCL_kernel_traits;

namespace factors::continuous {
    
    enum class KDEBandwidth {
        SCOTT
    };

    struct UnivariateKDE {

        template<typename ArrowType>
        void static execute_logpdf_mat(const cl::Buffer& training_vec, 
                                                    const unsigned int training_length,
                                                    const cl::Buffer& test_vec,
                                                    const unsigned int,
                                                    const unsigned int test_offset,
                                                    const unsigned int test_length,
                                                    const unsigned int,
                                                    const cl::Buffer& cholesky, 
                                                    const typename ArrowType::c_type lognorm_const,
                                                    cl::Buffer&,
                                                    cl::Buffer& output_mat);
    };

    template<typename ArrowType>
    void UnivariateKDE::execute_logpdf_mat(const cl::Buffer& training_vec, 
                                                   const unsigned int training_length,
                                                   const cl::Buffer& test_vec,
                                                   const unsigned int,
                                                   const unsigned int test_offset,
                                                   const unsigned int test_length,
                                                   const unsigned int,
                                                   const cl::Buffer& cholesky, 
                                                   const typename ArrowType::c_type lognorm_const,
                                                   cl::Buffer&,
                                                   cl::Buffer& output_mat) 
    {
        auto& opencl = OpenCLConfig::get();
        auto k_logpdf_values_1d_mat = opencl.kernel(OpenCL_kernel_traits<ArrowType>::logpdf_values_1d_mat);
        k_logpdf_values_1d_mat.setArg(0, training_vec);
        k_logpdf_values_1d_mat.setArg(1, training_length);
        k_logpdf_values_1d_mat.setArg(2, test_vec);
        k_logpdf_values_1d_mat.setArg(3, test_offset);
        k_logpdf_values_1d_mat.setArg(4, cholesky);
        k_logpdf_values_1d_mat.setArg(5, lognorm_const);
        k_logpdf_values_1d_mat.setArg(6, output_mat);
        auto queue = opencl.queue();        
        queue.enqueueNDRangeKernel(k_logpdf_values_1d_mat, cl::NullRange,  cl::NDRange(training_length*test_length), cl::NullRange);
    }

    struct MultivariateKDE {
        template<typename ArrowType>
        static void execute_logpdf_mat(const cl::Buffer& training_mat, 
                                        const unsigned int training_rows, 
                                        const cl::Buffer& test_mat,
                                        const unsigned int test_physical_rows,
                                        const unsigned int test_offset,
                                        const unsigned int test_length, 
                                        const unsigned int matrices_cols,
                                        const cl::Buffer& cholesky, 
                                        const typename ArrowType::c_type lognorm_const,
                                        cl::Buffer& tmp_mat,
                                        cl::Buffer& output_mat);
    };

    template<typename ArrowType>
    void MultivariateKDE::execute_logpdf_mat(const cl::Buffer& training_mat, 
                                                     const unsigned int training_rows, 
                                                     const cl::Buffer& test_mat,
                                                     const unsigned int test_physical_rows,
                                                     const unsigned int test_offset,
                                                     const unsigned int test_length, 
                                                     const unsigned int matrices_cols,
                                                     const cl::Buffer& cholesky, 
                                                     const typename ArrowType::c_type lognorm_const,
                                                     cl::Buffer& tmp_mat,
                                                     cl::Buffer& output_mat) 
    {
        auto& opencl = OpenCLConfig::get();
        
        auto k_substract = opencl.kernel(OpenCL_kernel_traits<ArrowType>::substract);

        auto k_solve = opencl.kernel(OpenCL_kernel_traits<ArrowType>::solve);
        k_solve.setArg(0, tmp_mat);
        k_solve.setArg(2, matrices_cols);
        k_solve.setArg(3, cholesky);

        auto k_square = opencl.kernel(OpenCL_kernel_traits<ArrowType>::square);
        k_square.setArg(0, tmp_mat);

        auto& queue = opencl.queue();

        if (training_rows > test_length) {
            k_substract.setArg(0, training_mat);
            k_substract.setArg(1, training_rows);
            k_substract.setArg(2, 0u);
            k_substract.setArg(3, training_rows);
            k_substract.setArg(4, test_mat);
            k_substract.setArg(5, test_physical_rows);
            k_substract.setArg(6, test_offset);
            k_substract.setArg(8, tmp_mat);

            k_solve.setArg(1, training_rows);
            
            auto k_logpdf_values_mat = opencl.kernel(OpenCL_kernel_traits<ArrowType>::logpdf_values_mat_column);
            k_logpdf_values_mat.setArg(0, tmp_mat);
            k_logpdf_values_mat.setArg(1, matrices_cols);
            k_logpdf_values_mat.setArg(2, output_mat);
            k_logpdf_values_mat.setArg(3, training_rows);
            k_logpdf_values_mat.setArg(5, lognorm_const);

            for (unsigned int i = 0; i < test_length; ++i) {
                k_substract.setArg(7, i);
                queue.enqueueNDRangeKernel(k_substract, cl::NullRange,  cl::NDRange(training_rows*matrices_cols), cl::NullRange);
                queue.enqueueNDRangeKernel(k_solve, cl::NullRange,  cl::NDRange(training_rows), cl::NullRange);
                queue.enqueueNDRangeKernel(k_square, cl::NullRange,  cl::NDRange(training_rows*matrices_cols), cl::NullRange);
                k_logpdf_values_mat.setArg(4, i);
                queue.enqueueNDRangeKernel(k_logpdf_values_mat, cl::NullRange,  cl::NDRange(training_rows), cl::NullRange);
            }
        } else {
            k_substract.setArg(0, test_mat);
            k_substract.setArg(1, test_physical_rows);
            k_substract.setArg(2, test_offset);
            k_substract.setArg(3, test_length);
            k_substract.setArg(4, training_mat);
            k_substract.setArg(5, training_rows);
            k_substract.setArg(6, 0);
            k_substract.setArg(8, tmp_mat);

            k_solve.setArg(1, test_length);

            auto k_logpdf_values_mat = opencl.kernel(OpenCL_kernel_traits<ArrowType>::logpdf_values_mat_row);
            k_logpdf_values_mat.setArg(0, tmp_mat);
            k_logpdf_values_mat.setArg(1, matrices_cols);
            k_logpdf_values_mat.setArg(2, output_mat);
            k_logpdf_values_mat.setArg(3, training_rows);
            k_logpdf_values_mat.setArg(5, lognorm_const);

            for(unsigned int i = 0; i < training_rows; ++i) {
                k_substract.setArg(7, i);
                queue.enqueueNDRangeKernel(k_substract, cl::NullRange,  cl::NDRange(test_length*matrices_cols), cl::NullRange);
                queue.enqueueNDRangeKernel(k_solve, cl::NullRange,  cl::NDRange(test_length), cl::NullRange);
                queue.enqueueNDRangeKernel(k_square, cl::NullRange,  cl::NDRange(test_length*matrices_cols), cl::NullRange);
                k_logpdf_values_mat.setArg(4, i);
                queue.enqueueNDRangeKernel(k_logpdf_values_mat, cl::NullRange,  cl::NDRange(test_length), cl::NullRange);

            }
        }
    }
    
    class KDE {
    public:

        KDE() : m_variables(), 
                m_bselector(KDEBandwidth::SCOTT), 
                m_H_cholesky(), 
                m_training(), 
                m_lognorm_const(0), 
                N(0), 
                m_training_type(arrow::Type::type::NA) {}

        KDE(std::vector<std::string> variables) : KDE(variables, KDEBandwidth::SCOTT) {
            if (m_variables.empty()) {
                throw std::invalid_argument("Cannot create a KDE model with 0 variables");
            }
        }

        KDE(std::vector<std::string> variables, KDEBandwidth b_selector) : m_variables(variables), 
                                                                            m_bselector(b_selector),
                                                                            m_H_cholesky(),
                                                                            m_training(),
                                                                            m_lognorm_const(0),
                                                                            N(0),
                                                                            m_training_type(arrow::Type::type::NA) {
            if (m_variables.empty()) {
                throw std::invalid_argument("Cannot create a KDE model with 0 variables");
            }
        }

        void fit(py::handle pyobject);
        void fit(const DataFrame& df);

        template<typename ArrowType, typename EigenMatrix>
        void fit(EigenMatrix bandwidth, cl::Buffer training_data, arrow::Type::type training_type, int training_instances);

        const MatrixXd& bandwidth() const { return m_bandwidth; }
        cl::Buffer& training_buffer() { return m_training; }
        int num_instances() const { return N; }
        arrow::Type::type data_type() const { return m_training_type; }
        KDEBandwidth bandwidth_type() const { return m_bselector; }

        VectorXd logpdf(py::handle pyobject) const;
        VectorXd logpdf(const DataFrame& df) const;

        template<typename ArrowType>
        cl::Buffer logpdf_buffer(const DataFrame& df) const;

        double slogpdf(py::handle pyobject) const;
        double slogpdf(const DataFrame& df) const;

    private:
        template<typename ArrowType, bool contains_null>
        void _fit(const DataFrame& df);

        template<typename ArrowType>
        VectorXd _logpdf(const DataFrame& df) const;
        template<typename ArrowType>
        double _slogpdf(const DataFrame& df) const;


        template<typename ArrowType, typename KDEType>
        cl::Buffer _logpdf_impl(const DataFrame& df) const;

        template<typename ArrowType>
        std::pair<cl::Buffer, uint64_t> allocate_mat(uint64_t m) const;

        template<typename ArrowType, bool contains_null>
        void compute_bandwidth(const DataFrame& df, std::vector<std::string>& variables);

        std::vector<std::string> m_variables;
        KDEBandwidth m_bselector;
        MatrixXd m_bandwidth;
        cl::Buffer m_H_cholesky;
        cl::Buffer m_training;
        double m_lognorm_const;
        size_t N;
        arrow::Type::type m_training_type;
    };

    template<typename ArrowType, bool contains_null>
    void KDE::compute_bandwidth(const DataFrame& df, std::vector<std::string>& variables) {
        using CType = typename ArrowType::c_type;
        auto cov = df.cov<ArrowType, contains_null>(variables);

        if constexpr(std::is_same_v<ArrowType, arrow::DoubleType>) {
            m_bandwidth = std::pow(static_cast<CType>(df.valid_count(variables)), -2 / static_cast<CType>(variables.size() + 4)) * (*cov);
        } else {
            m_bandwidth = std::pow(static_cast<CType>(df.valid_count(variables)), -2 / static_cast<CType>(variables.size() + 4)) * 
                                (cov->template cast<double>());
        }
    }

    template<typename ArrowType, bool contains_null>
    void KDE::_fit(const DataFrame& df) {
        using CType = typename ArrowType::c_type;

        auto d = m_variables.size();

        compute_bandwidth<ArrowType, contains_null>(df, m_variables);

        auto llt_cov = m_bandwidth.llt();
        auto llt_matrix = llt_cov.matrixLLT();

        auto& opencl = OpenCLConfig::get();
        m_H_cholesky = opencl.copy_to_buffer(llt_matrix.data(), d*d);
        
        auto training_data = df.to_eigen<false, ArrowType, contains_null>(m_variables);
        N = training_data->rows();
        m_training = opencl.copy_to_buffer(training_data->data(), N * d);

        m_lognorm_const = -llt_matrix.diagonal().array().log().sum() 
                          - 0.5 * d * std::log(2*util::pi<CType>)
                          - std::log(N);
    }

    template<typename ArrowType, typename EigenMatrix>
    void KDE::fit(EigenMatrix bandwidth, cl::Buffer training_data, arrow::Type::type training_type, int training_instances) {
        using CType = typename ArrowType::c_type;

        if ((bandwidth.rows() != bandwidth.cols()) || (static_cast<size_t>(bandwidth.rows()) != m_variables.size())) {
            throw std::invalid_argument("Bandwidth matrix must be a square matrix with dimensionality " + std::to_string(m_variables.size()));
        }
        
        m_bandwidth = bandwidth;
        auto d = m_variables.size();
        auto llt_cov = bandwidth.llt();
        auto cholesky = llt_cov.matrixLLT();
        auto& opencl = OpenCLConfig::get();
        m_H_cholesky = opencl.copy_to_buffer(cholesky.data(), d*d);
        m_training = training_data;
        m_training_type = training_type;
        N = training_instances;
        m_lognorm_const = -cholesky.diagonal().array().log().sum() 
                          - 0.5 * d * std::log(2*util::pi<CType>)
                          - std::log(N);
    }

    template<typename ArrowType>
    VectorXd KDE::_logpdf(const DataFrame& df) const {
        using CType = typename ArrowType::c_type;
        using VectorType = Matrix<CType, Dynamic, 1>;

        auto logpdf_buff = logpdf_buffer<ArrowType>(df);
        auto& opencl = OpenCLConfig::get();
        
        if (df.null_count(m_variables) == 0) {
            VectorType read_data(df->num_rows());
            opencl.read_from_buffer(read_data.data(), logpdf_buff, df->num_rows());
            if constexpr (!std::is_same_v<CType, double>)
                return read_data.template cast<double>();
            else
                return read_data;
        } else {
            auto m = df.valid_count(m_variables);
            VectorType read_data(m);
            auto bitmap = df.combined_bitmap(m_variables);
            auto bitmap_data = bitmap->data();

            opencl.read_from_buffer(read_data.data(), logpdf_buff, m);

            VectorXd res(df->num_rows());

            for (int i = 0, k = 0; i < df->num_rows(); ++i) {
                if(arrow::BitUtil::GetBit(bitmap_data, i)) {
                    res(i) = static_cast<double>(read_data[k++]);
                } else {
                    res(i) = util::nan<double>;
                }
            }

            return res;
        }
    }

    template<typename ArrowType>
    double KDE::_slogpdf(const DataFrame& df) const {
        using CType = typename ArrowType::c_type;

        auto logpdf_buff = logpdf_buffer<ArrowType>(df);
        auto m = df.valid_count(m_variables);

        auto& opencl = OpenCLConfig::get();
        auto buffer_sum = opencl.sum1d<ArrowType>(logpdf_buff, m);

        CType result = 0;
        opencl.read_from_buffer(&result, buffer_sum, 1);
        return static_cast<double>(result);
    }

    template<typename ArrowType>
    cl::Buffer KDE::logpdf_buffer(const DataFrame& df) const {
        if (m_variables.size() == 1)
            return _logpdf_impl<ArrowType, UnivariateKDE>(df);
        else
            return _logpdf_impl<ArrowType, MultivariateKDE>(df);
    }

    template<typename ArrowType>
    std::pair<cl::Buffer, uint64_t> KDE::allocate_mat(uint64_t m) const {
        using CType = typename ArrowType::c_type;
        auto& opencl = OpenCLConfig::get();
        auto& device = opencl.device();

        cl_int err_code = CL_SUCCESS;

        uint64_t max_alloc = device.getInfo<CL_DEVICE_MAX_MEM_ALLOC_SIZE>(&err_code);
        if(err_code != CL_SUCCESS) {
            throw std::runtime_error(std::string("Error while querying device info: ") +
                                     opencl::opencl_error(err_code) + " (" + std::to_string(err_code) + ").");
        }

        double work_factor = std::ceil(static_cast<double>(static_cast<uint64_t>(N)*static_cast<uint64_t>(m)*sizeof(CType))
                                                            / static_cast<double>(max_alloc));

        uint64_t allocatable_m = std::ceil(static_cast<double>(m) / work_factor);
        // return std::make_pair(opencl.new_buffer<CType>(N*allocatable_m), allocatable_m);

        // FIXME: Benchmark what size increases the performance.
        return std::make_pair(opencl.new_buffer<CType>(N*64), 64);
    }

    template<typename ArrowType, typename KDEType>
    cl::Buffer KDE::_logpdf_impl(const DataFrame& df) const {
        using CType = typename ArrowType::c_type;
        auto d = m_variables.size();
        auto& opencl = OpenCLConfig::get();

        auto test_matrix = df.to_eigen<false, ArrowType>(m_variables);
        auto m = test_matrix->rows();
        auto test_buffer = opencl.copy_to_buffer(test_matrix->data(), m*d);

        auto res = opencl.new_buffer<CType>(m);

        auto [mat_logpdfs, allocated_m] = allocate_mat<ArrowType>(m);
        auto iterations = std::ceil(static_cast<double>(m) / static_cast<double>(allocated_m));

        cl::Buffer tmp_mat_buffer;
        if constexpr(std::is_same_v<KDEType, MultivariateKDE>) {
            if (N > allocated_m)
                tmp_mat_buffer = opencl.new_buffer<CType>(N*m_variables.size());
            else
                tmp_mat_buffer = opencl.new_buffer<CType>(allocated_m*m_variables.size());
        }

        auto reduc_buffers = opencl.create_reduction_mat_buffers<ArrowType>(N, allocated_m);

        for (auto i = 0; i < (iterations-1); ++i) {
            KDEType::template execute_logpdf_mat<ArrowType>(m_training, N, test_buffer, m, 
                                                        i*allocated_m, allocated_m, d, m_H_cholesky, m_lognorm_const, 
                                                        tmp_mat_buffer, mat_logpdfs);
            opencl.logsumexp_cols_offset<ArrowType>(mat_logpdfs, N, allocated_m, res, i*allocated_m, reduc_buffers);
        }
        auto remaining_m = m - (iterations - 1)*allocated_m;
        KDEType::template execute_logpdf_mat<ArrowType>(m_training, N, test_buffer, m, 
                                                        m - remaining_m, remaining_m, d, m_H_cholesky, m_lognorm_const, 
                                                        tmp_mat_buffer, mat_logpdfs);
        opencl.logsumexp_cols_offset<ArrowType>(mat_logpdfs, N, remaining_m, res, (iterations - 1)*allocated_m, reduc_buffers);


        return res;
    }


    class CKDE {
    public:
        CKDE(const std::string variable, const std::vector<std::string> evidence) : CKDE(variable, evidence, KDEBandwidth::SCOTT) {}
        CKDE(const std::string variable, const std::vector<std::string> evidence, KDEBandwidth b_selector) 
                                                                                  : m_variable(variable), 
                                                                                    m_evidence(evidence),
                                                                                    m_variables(),
                                                                                    m_fitted(false),
                                                                                    m_bselector(b_selector),
                                                                                    m_training_type(arrow::Type::type::NA),
                                                                                    m_joint(),
                                                                                    m_marg() {
            
            m_variables.reserve(evidence.size() + 1);
            m_variables.push_back(variable);
            for (auto it = evidence.begin(); it != evidence.end(); ++it) {
                m_variables.push_back(*it);
            }
        }

        const std::string& variable() const { return m_variable; }
        const std::vector<std::string>& evidence() const { return m_evidence; }
        bool fitted() const { return m_fitted; }

        void fit(py::handle pyobject);
        void fit(const DataFrame& df);

        VectorXd logpdf(py::handle pyobject) const;
        VectorXd logpdf(const DataFrame& df) const;

        double slogpdf(py::handle pyobject) const;
        double slogpdf(const DataFrame& df) const;

        std::string ToString() const;
    private:
        template<typename ArrowType>
        VectorXd _logpdf(const DataFrame& df) const;

        template<typename ArrowType>
        double _slogpdf(const DataFrame& df) const;

        template<typename ArrowType>
        void _fit(const DataFrame& df);

        std::string m_variable;
        std::vector<std::string> m_evidence;
        std::vector<std::string> m_variables;
        bool m_fitted;
        KDEBandwidth m_bselector;
        arrow::Type::type m_training_type;
        size_t N;
        KDE m_joint;
        KDE m_marg;
    };

    template<typename ArrowType>
    void CKDE::_fit(const DataFrame& df) {
        m_joint = KDE(m_variables);
        m_joint.fit(df);
        N = m_joint.num_instances();

        if (!m_evidence.empty()) {
            m_marg = KDE(m_evidence);
            auto& joint_bandwidth = m_joint.bandwidth();
            auto d = m_variables.size();
            auto marg_bandwidth = joint_bandwidth.bottomRightCorner(d-1, d-1);

            cl::Buffer& training_buffer = m_joint.training_buffer();

            auto& opencl = OpenCLConfig::get();
            using CType = typename ArrowType::c_type;
            auto marg_buffer = opencl.copy_buffer<CType>(training_buffer, N, N*(d-1));

            m_marg.fit<ArrowType>(marg_bandwidth, marg_buffer, m_joint.data_type(), N);
        }
    }

    template<typename ArrowType>
    VectorXd CKDE::_logpdf(const DataFrame& df) const {
        using CType = typename ArrowType::c_type;
        using VectorType = Matrix<CType, Dynamic, 1>;

        auto logpdf_joint = m_joint.logpdf_buffer<ArrowType>(df);

        auto& opencl = OpenCLConfig::get();
        if (!m_evidence.empty()) {
            auto logpdf_marg = m_marg.logpdf_buffer<ArrowType>(df);
            auto m = df.valid_count(m_variables);

            
            auto k_substract = opencl.kernel(OpenCL_kernel_traits<ArrowType>::substract_vectors);
            k_substract.setArg(0, logpdf_joint);
            k_substract.setArg(1, logpdf_marg);
            auto& queue = opencl.queue();
            queue.enqueueNDRangeKernel(k_substract, cl::NullRange,  cl::NDRange(m), cl::NullRange);
        }

        if (df.null_count(m_variables) == 0) {
            VectorType read_data(df->num_rows());
            opencl.read_from_buffer(read_data.data(), logpdf_joint, df->num_rows());
            if constexpr (!std::is_same_v<CType, double>)
                return read_data.template cast<double>();
            else
                return read_data;
        } else {
            auto m = df.valid_count(m_variables);
            VectorType read_data(m);
            auto bitmap = df.combined_bitmap(m_variables);
            auto bitmap_data = bitmap->data();

            opencl.read_from_buffer(read_data.data(), logpdf_joint, m);

            VectorXd res(df->num_rows());

            for (int i = 0, k = 0; i < df->num_rows(); ++i) {
                if(arrow::BitUtil::GetBit(bitmap_data, i)) {
                    res(i) = static_cast<double>(read_data[k++]);
                } else {
                    res(i) = util::nan<double>;
                }
            }

            return res;
        }
    }

    template<typename ArrowType>
    double CKDE::_slogpdf(const DataFrame& df) const {
        using CType = typename ArrowType::c_type;

        auto logpdf_joint = m_joint.logpdf_buffer<ArrowType>(df);
        auto m = df.valid_count(m_variables);

        auto& opencl = OpenCLConfig::get();
        if(!m_evidence.empty()) {
            auto logpdf_marg = m_marg.logpdf_buffer<ArrowType>(df);
            
            auto k_substract = opencl.kernel(OpenCL_kernel_traits<ArrowType>::substract_vectors);
            k_substract.setArg(0, logpdf_joint);
            k_substract.setArg(1, logpdf_marg);
            auto& queue = opencl.queue();
            queue.enqueueNDRangeKernel(k_substract, cl::NullRange,  cl::NDRange(m), cl::NullRange);
        }

        auto buffer_sum = opencl.sum1d<ArrowType>(logpdf_joint, m);

        CType result = 0;
        opencl.read_from_buffer(&result, buffer_sum, 1);
        return static_cast<double>(result);
    }

}

#endif //PGM_DATASET_CKDE_HPP