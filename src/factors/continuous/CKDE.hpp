#ifndef PGM_DATASET_CKDE_HPP
#define PGM_DATASET_CKDE_HPP

#include <random>
#include <CL/cl2.hpp>
#include <opencl/opencl_config.hpp>
#include <pybind11/pybind11.h>
#include <Eigen/Dense>
#include <pybind11/eigen.h>
#include <dataset/dataset.hpp>
#include <util/math_constants.hpp>
#include <util/bit_util.hpp>

namespace py = pybind11;
using dataset::DataFrame;
    using Eigen::VectorXd, Eigen::VectorXi, Eigen::Ref, Eigen::LLT;
using opencl::OpenCLConfig, opencl::OpenCL_kernel_traits;

namespace factors::continuous {
    
    enum class KDEBandwidth {
        SCOTT,
        MANUAL
    };

    struct UnivariateKDE {

        template<typename ArrowType>
        void static execute_logl_mat(const cl::Buffer& training_vec, 
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
        template<typename ArrowType>
        static void execute_conditional_means(const cl::Buffer& joint_training,
                                                const cl::Buffer&,
                                                const unsigned int training_rows, 
                                                const cl::Buffer& evidence_test,
                                                const unsigned int test_physical_rows,
                                                const unsigned int test_offset,
                                                const unsigned int test_length, 
                                                const unsigned int,
                                                const cl::Buffer& transform_mean,
                                                cl::Buffer&,
                                                cl::Buffer& output_mat);
    };

    template<typename ArrowType>
    void UnivariateKDE::execute_logl_mat(const cl::Buffer& training_vec, 
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
        auto k_logl_values_1d_mat = opencl.kernel(OpenCL_kernel_traits<ArrowType>::logl_values_1d_mat);
        k_logl_values_1d_mat.setArg(0, training_vec);
        k_logl_values_1d_mat.setArg(1, training_length);
        k_logl_values_1d_mat.setArg(2, test_vec);
        k_logl_values_1d_mat.setArg(3, test_offset);
        k_logl_values_1d_mat.setArg(4, cholesky);
        k_logl_values_1d_mat.setArg(5, lognorm_const);
        k_logl_values_1d_mat.setArg(6, output_mat);
        auto queue = opencl.queue();        
        queue.enqueueNDRangeKernel(k_logl_values_1d_mat, cl::NullRange,  cl::NDRange(training_length*test_length), cl::NullRange);
    }

    template<typename ArrowType>
    void UnivariateKDE::execute_conditional_means(const cl::Buffer& joint_training,
                                                            const cl::Buffer&,
                                                            const unsigned int training_rows, 
                                                            const cl::Buffer& evidence_test,
                                                            const unsigned int test_physical_rows,
                                                            const unsigned int test_offset,
                                                            const unsigned int test_length, 
                                                            const unsigned int,
                                                            const cl::Buffer& transform_mean,
                                                            cl::Buffer&,
                                                            cl::Buffer& output_mat) {
        auto& opencl = OpenCLConfig::get();
        auto k_conditional_means_1d = opencl.kernel(OpenCL_kernel_traits<ArrowType>::conditional_means_1d);
        k_conditional_means_1d.setArg(0, joint_training);
        k_conditional_means_1d.setArg(1, training_rows);
        k_conditional_means_1d.setArg(2, evidence_test);
        k_conditional_means_1d.setArg(3, test_physical_rows);
        k_conditional_means_1d.setArg(4, test_offset);
        k_conditional_means_1d.setArg(5, transform_mean);
        k_conditional_means_1d.setArg(6, output_mat);
        auto queue = opencl.queue();        
        queue.enqueueNDRangeKernel(k_conditional_means_1d, cl::NullRange,  cl::NDRange(training_rows*test_length), cl::NullRange);
    }

    struct MultivariateKDE {
        template<typename ArrowType>
        static void execute_logl_mat(const cl::Buffer& training_mat, 
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

        template<typename ArrowType>
        static void execute_conditional_means(const cl::Buffer& joint_training,
                                                const cl::Buffer& marg_training,
                                                const unsigned int training_rows, 
                                                const cl::Buffer& evidence_test,
                                                const unsigned int test_physical_rows,
                                                const unsigned int test_offset,
                                                const unsigned int test_length, 
                                                const unsigned int evidence_cols,
                                                const cl::Buffer& transform_mean,
                                                cl::Buffer& tmp_mat,
                                                cl::Buffer& output_mat);
    };

    template<typename ArrowType>
    void MultivariateKDE::execute_logl_mat(const cl::Buffer& training_mat, 
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
            
            auto k_logl_values_mat = opencl.kernel(OpenCL_kernel_traits<ArrowType>::logl_values_mat_column);
            k_logl_values_mat.setArg(0, tmp_mat);
            k_logl_values_mat.setArg(1, matrices_cols);
            k_logl_values_mat.setArg(2, output_mat);
            k_logl_values_mat.setArg(3, training_rows);
            k_logl_values_mat.setArg(5, lognorm_const);

            for (unsigned int i = 0; i < test_length; ++i) {
                k_substract.setArg(7, i);
                queue.enqueueNDRangeKernel(k_substract, cl::NullRange,  cl::NDRange(training_rows*matrices_cols), cl::NullRange);
                queue.enqueueNDRangeKernel(k_solve, cl::NullRange,  cl::NDRange(training_rows), cl::NullRange);
                queue.enqueueNDRangeKernel(k_square, cl::NullRange,  cl::NDRange(training_rows*matrices_cols), cl::NullRange);
                k_logl_values_mat.setArg(4, i);
                queue.enqueueNDRangeKernel(k_logl_values_mat, cl::NullRange,  cl::NDRange(training_rows), cl::NullRange);
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

            auto k_logl_values_mat = opencl.kernel(OpenCL_kernel_traits<ArrowType>::logl_values_mat_row);
            k_logl_values_mat.setArg(0, tmp_mat);
            k_logl_values_mat.setArg(1, matrices_cols);
            k_logl_values_mat.setArg(2, output_mat);
            k_logl_values_mat.setArg(3, training_rows);
            k_logl_values_mat.setArg(5, lognorm_const);

            for(unsigned int i = 0; i < training_rows; ++i) {
                k_substract.setArg(7, i);
                queue.enqueueNDRangeKernel(k_substract, cl::NullRange,  cl::NDRange(test_length*matrices_cols), cl::NullRange);
                queue.enqueueNDRangeKernel(k_solve, cl::NullRange,  cl::NDRange(test_length), cl::NullRange);
                queue.enqueueNDRangeKernel(k_square, cl::NullRange,  cl::NDRange(test_length*matrices_cols), cl::NullRange);
                k_logl_values_mat.setArg(4, i);
                queue.enqueueNDRangeKernel(k_logl_values_mat, cl::NullRange,  cl::NDRange(test_length), cl::NullRange);
            }
        }
    }

    template<typename ArrowType>
    void MultivariateKDE::execute_conditional_means(const cl::Buffer& joint_training,
                                                            const cl::Buffer& marg_training,
                                                            const unsigned int training_rows, 
                                                            const cl::Buffer& evidence_test,
                                                            const unsigned int test_physical_rows,
                                                            const unsigned int test_offset,
                                                            const unsigned int test_length, 
                                                            const unsigned int evidence_cols,
                                                            const cl::Buffer& transform_mean,
                                                            cl::Buffer& tmp_mat,
                                                            cl::Buffer& output_mat) {

        auto& opencl = OpenCLConfig::get();
        
        auto k_substract = opencl.kernel(OpenCL_kernel_traits<ArrowType>::substract);
        auto& queue = opencl.queue();

        if (training_rows > test_length) {
            k_substract.setArg(0, marg_training);
            k_substract.setArg(1, training_rows);
            k_substract.setArg(2, 0u);
            k_substract.setArg(3, training_rows);
            k_substract.setArg(4, evidence_test);
            k_substract.setArg(5, test_physical_rows);
            k_substract.setArg(6, test_offset);
            k_substract.setArg(8, tmp_mat);

            auto k_conditional_means = opencl.kernel(OpenCL_kernel_traits<ArrowType>::conditional_means_column);

            k_conditional_means.setArg(0, joint_training);
            k_conditional_means.setArg(1, static_cast<unsigned int>(training_rows));
            k_conditional_means.setArg(2, tmp_mat);
            k_conditional_means.setArg(3, static_cast<unsigned int>(training_rows));
            k_conditional_means.setArg(4, transform_mean);
            k_conditional_means.setArg(5, static_cast<unsigned int>(evidence_cols));
            k_conditional_means.setArg(6, output_mat);
            k_conditional_means.setArg(8, static_cast<unsigned int>(training_rows));

            for (unsigned int i = 0; i < test_length; ++i) {
                k_substract.setArg(7, i);
                queue.enqueueNDRangeKernel(k_substract, cl::NullRange,  cl::NDRange(training_rows*evidence_cols), cl::NullRange);
                k_conditional_means.setArg(7, i);
                queue.enqueueNDRangeKernel(k_conditional_means, cl::NullRange,  cl::NDRange(training_rows), cl::NullRange);

            }
        } else {
            k_substract.setArg(0, evidence_test);
            k_substract.setArg(1, test_physical_rows);
            k_substract.setArg(2, test_offset);
            k_substract.setArg(3, test_length);
            k_substract.setArg(4, marg_training);
            k_substract.setArg(5, training_rows);
            k_substract.setArg(6, 0);
            k_substract.setArg(8, tmp_mat);

            auto k_conditional_means = opencl.kernel(OpenCL_kernel_traits<ArrowType>::conditional_means_row);

            k_conditional_means.setArg(0, joint_training);
            k_conditional_means.setArg(1, static_cast<unsigned int>(training_rows));
            k_conditional_means.setArg(2, tmp_mat);
            k_conditional_means.setArg(3, static_cast<unsigned int>(test_length));
            k_conditional_means.setArg(4, transform_mean);
            k_conditional_means.setArg(5, static_cast<unsigned int>(evidence_cols));
            k_conditional_means.setArg(6, output_mat);
            k_conditional_means.setArg(8, static_cast<unsigned int>(test_length));

            for(unsigned int i = 0; i < training_rows; ++i) {
                k_substract.setArg(7, i);
                queue.enqueueNDRangeKernel(k_substract, cl::NullRange,  cl::NDRange(test_length*evidence_cols), cl::NullRange);
                k_conditional_means.setArg(7, i);
                queue.enqueueNDRangeKernel(k_conditional_means, cl::NullRange,  cl::NDRange(test_length), cl::NullRange);
            }
        }
    }

    template<typename ArrowType>
    std::pair<cl::Buffer, uint64_t> allocate_mat(int rows) {
        using CType = typename ArrowType::c_type;
        auto& opencl = OpenCLConfig::get();
        return std::make_pair(opencl.new_buffer<CType>(rows*64), 64);
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
                                                                            m_fitted(false),
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

        const std::vector<std::string>& variables() const { return m_variables; }
        void fit(const DataFrame& df);

        template<typename ArrowType, typename EigenMatrix>
        void fit(EigenMatrix bandwidth, cl::Buffer training_data, arrow::Type::type training_type, int training_instances);

        const MatrixXd& bandwidth() const { return m_bandwidth; }
        void setBandwidth(MatrixXd& new_bandwidth) { 
            if (new_bandwidth.rows() != new_bandwidth.cols() || static_cast<size_t>(new_bandwidth.rows()) != m_variables.size())
                throw std::invalid_argument("The bandwidth matrix must be a square matrix with shape "
                            "(" + std::to_string(m_variables.size()) + ", " + std::to_string(m_variables.size()) + ")"); 
            
            m_bandwidth = new_bandwidth;
            if (m_bandwidth.rows() > 0)
                copy_bandwidth_opencl();
            m_bselector = KDEBandwidth::MANUAL;
        }
        
        cl::Buffer& training_buffer() { return m_training; }
        const cl::Buffer& training_buffer() const { return m_training; }

        cl::Buffer& cholesky_buffer()  { return m_H_cholesky; }
        const cl::Buffer& cholesky_buffer() const { return m_H_cholesky; }

        double lognorm_const() const { return m_lognorm_const; }

        DataFrame training_data() const;
        
        int num_instances() const { return N; }
        int num_variables() const { return m_variables.size(); }
        bool fitted() const { return m_fitted; }

        arrow::Type::type data_type() const { return m_training_type; }
        KDEBandwidth bandwidth_type() const { return m_bselector; }

        VectorXd logl(const DataFrame& df) const;

        template<typename ArrowType>
        cl::Buffer logl_buffer(const DataFrame& df) const;
        template<typename ArrowType>
        cl::Buffer logl_buffer(const DataFrame& df, Buffer_ptr& bitmap) const;

        double slogl(const DataFrame& df) const;
    private:
        template<typename ArrowType>
        DataFrame _training_data() const;

        template<typename ArrowType, bool contains_null>
        void _fit(const DataFrame& df);

        template<typename ArrowType>
        VectorXd _logl(const DataFrame& df) const;
        template<typename ArrowType>
        double _slogl(const DataFrame& df) const;

        template<typename ArrowType, typename KDEType>
        cl::Buffer _logl_impl(cl::Buffer& test_buffer, int m) const;

        template<typename ArrowType, bool contains_null>
        void compute_bandwidth(const DataFrame& df, std::vector<std::string>& variables);

        void copy_bandwidth_opencl();

        std::vector<std::string> m_variables;
        bool m_fitted;
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
            m_bandwidth = std::pow(static_cast<CType>(df.valid_rows(variables)), -2 / static_cast<CType>(variables.size() + 4)) * (*cov);
        } else {
            m_bandwidth = std::pow(static_cast<CType>(df.valid_rows(variables)), -2 / static_cast<CType>(variables.size() + 4)) * 
                                (cov->template cast<double>());
        }
    }

    template<typename ArrowType>
    DataFrame KDE::_training_data() const {
        using CType = typename ArrowType::c_type;
        using VectorType = Matrix<CType, Dynamic, 1>;
        arrow::NumericBuilder<ArrowType> builder;

        auto& opencl = OpenCLConfig::get();
        VectorType tmp_buffer(N*m_variables.size());
        opencl.read_from_buffer(tmp_buffer.data(), m_training, N*m_variables.size());

        std::vector<Array_ptr> columns;
        arrow::SchemaBuilder b(arrow::SchemaBuilder::ConflictPolicy::CONFLICT_ERROR);
        for (int i = 0; i < m_variables.size(); ++i) {
            builder.Resize(N);
            auto status = builder.AppendValues(tmp_buffer.data() + i*N, N);

            if (!status.ok()) {
                throw std::runtime_error("Values could not be added. Error status: " + status.ToString());
            }

            Array_ptr out;
            status = builder.Finish(&out);
            if (!status.ok()) {
               throw std::runtime_error("New array could not be created. Error status: " + status.ToString());
            }

            columns.push_back(out);
            builder.Reset();

            auto f = arrow::field(m_variables[i], out->type());
            status = b.AddField(f);
            if (!status.ok()) {
                throw std::runtime_error("Field could not be added to the Schema. Error status: " + status.ToString());
            }
        }

        auto r = b.Finish();
        if (!r.ok()) {
            throw std::domain_error("Schema could not be created.");
        }
        auto schema = std::move(r).ValueOrDie();

        auto rb = arrow::RecordBatch::Make(schema, N, columns);
        return DataFrame(rb);
    }


    template<typename ArrowType, bool contains_null>
    void KDE::_fit(const DataFrame& df) {
        using CType = typename ArrowType::c_type;

        auto d = m_variables.size();

        compute_bandwidth<ArrowType, contains_null>(df, m_variables);

        auto llt_cov = m_bandwidth.llt();
        auto llt_matrix = llt_cov.matrixLLT();

        auto& opencl = OpenCLConfig::get();

        if constexpr(std::is_same_v<CType, double>) {
            m_H_cholesky = opencl.copy_to_buffer(llt_matrix.data(), d*d);
        } else {
            using MatrixType = Matrix<CType, Dynamic, Dynamic>;
            MatrixType casted_cholesky = llt_matrix.template cast<CType>();
            m_H_cholesky = opencl.copy_to_buffer(casted_cholesky.data(), d*d);
        }

        auto training_data = df.to_eigen<false, ArrowType, contains_null>(m_variables);
        N = training_data->rows();
        m_training = opencl.copy_to_buffer(training_data->data(), N * d);

        m_lognorm_const = -llt_matrix.diagonal().array().log().sum() 
                          - 0.5 * d * std::log(2*util::pi<double>)
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

        if constexpr (std::is_same_v<CType, double>) {
            m_H_cholesky = opencl.copy_to_buffer(cholesky.data(), d*d);
        } else {
            using MatrixType = Matrix<CType, Dynamic, Dynamic>;
            MatrixType casted_cholesky = cholesky.template cast<CType>();
            m_H_cholesky = opencl.copy_to_buffer(casted_cholesky.data(), d*d);
        }

        m_training = training_data;
        m_training_type = training_type;
        N = training_instances;
        m_lognorm_const = -cholesky.diagonal().array().log().sum() 
                          - 0.5 * d * std::log(2*util::pi<double>)
                          - std::log(N);
    }

    template<typename ArrowType>
    VectorXd KDE::_logl(const DataFrame& df) const {
        using CType = typename ArrowType::c_type;
        using VectorType = Matrix<CType, Dynamic, 1>;

        auto logl_buff = logl_buffer<ArrowType>(df);
        auto& opencl = OpenCLConfig::get();
        if (df.null_count(m_variables) == 0) {
            VectorType read_data(df->num_rows());
            opencl.read_from_buffer(read_data.data(), logl_buff, df->num_rows());
            if constexpr (!std::is_same_v<CType, double>)
                return read_data.template cast<double>();
            else
                return read_data;
        } else {
            auto m = df.valid_rows(m_variables);
            VectorType read_data(m);
            auto bitmap = df.combined_bitmap(m_variables);
            auto bitmap_data = bitmap->data();

            opencl.read_from_buffer(read_data.data(), logl_buff, m);

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
    double KDE::_slogl(const DataFrame& df) const {
        using CType = typename ArrowType::c_type;

        auto logl_buff = logl_buffer<ArrowType>(df);
        auto m = df.valid_rows(m_variables);

        auto& opencl = OpenCLConfig::get();
        auto buffer_sum = opencl.sum1d<ArrowType>(logl_buff, m);

        CType result = 0;
        opencl.read_from_buffer(&result, buffer_sum, 1);
        return static_cast<double>(result);
    }

    template<typename ArrowType>
    cl::Buffer KDE::logl_buffer(const DataFrame& df) const {
        auto& opencl = OpenCLConfig::get();

        auto test_matrix = df.to_eigen<false, ArrowType>(m_variables);
        auto m = test_matrix->rows();
        auto test_buffer = opencl.copy_to_buffer(test_matrix->data(), m*m_variables.size());

        if (m_variables.size() == 1)
            return _logl_impl<ArrowType, UnivariateKDE>(test_buffer, m);
        else
            return _logl_impl<ArrowType, MultivariateKDE>(test_buffer, m);
    }

    template<typename ArrowType>
    cl::Buffer KDE::logl_buffer(const DataFrame& df, Buffer_ptr& bitmap) const {
        auto& opencl = OpenCLConfig::get();

        auto test_matrix = df.to_eigen<false, ArrowType>(bitmap, m_variables);
        auto m = test_matrix->rows();
        auto test_buffer = opencl.copy_to_buffer(test_matrix->data(), m*m_variables.size());

        if (m_variables.size() == 1)
            return _logl_impl<ArrowType, UnivariateKDE>(test_buffer, m);
        else
            return _logl_impl<ArrowType, MultivariateKDE>(test_buffer, m);
    }

    template<typename ArrowType, typename KDEType>
    cl::Buffer KDE::_logl_impl(cl::Buffer& test_buffer, int m) const {
        using CType = typename ArrowType::c_type;
        auto d = m_variables.size();
        auto& opencl = OpenCLConfig::get();
        auto res = opencl.new_buffer<CType>(m);

        auto [mat_logls, allocated_m] = allocate_mat<ArrowType>(N);
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
            KDEType::template execute_logl_mat<ArrowType>(m_training, N, test_buffer, m, 
                                                        i*allocated_m, allocated_m, d, m_H_cholesky, m_lognorm_const, 
                                                        tmp_mat_buffer, mat_logls);
            opencl.logsumexp_cols_offset<ArrowType>(mat_logls, N, allocated_m, res, i*allocated_m, reduc_buffers);
        }
        auto remaining_m = m - (iterations - 1)*allocated_m;
        KDEType::template execute_logl_mat<ArrowType>(m_training, N, test_buffer, m, 
                                                        m - remaining_m, remaining_m, d, m_H_cholesky, m_lognorm_const, 
                                                        tmp_mat_buffer, mat_logls);
        opencl.logsumexp_cols_offset<ArrowType>(mat_logls, N, remaining_m, res, (iterations - 1)*allocated_m, reduc_buffers);

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

            m_joint = KDE(m_variables);
            if (!m_evidence.empty()) {
                m_marg = KDE(m_evidence);
            }
        }

        const std::string& variable() const { return m_variable; }
        const std::vector<std::string>& evidence() const { return m_evidence; }
        int num_instances() const { return N; }

        KDE& kde_joint() { return m_joint; }
        KDE& kde_marg() { return m_marg; }

        bool fitted() const { return m_fitted; }

        arrow::Type::type data_type() const { return m_training_type; }
        KDEBandwidth bandwidth_type() const { return m_bselector; }

        void fit(const DataFrame& df);
        VectorXd logl(const DataFrame& df) const;
        double slogl(const DataFrame& df) const;

        Array_ptr sample(int n, 
                         const DataFrame& evidence_values, 
                         long unsigned int seed = std::random_device{}()) const;
        
        VectorXd cdf(const DataFrame& df) const;

        std::string ToString() const;
    private:
        template<typename ArrowType>
        void _fit(const DataFrame& df);

        template<typename ArrowType>
        VectorXd _logl(const DataFrame& df) const;

        template<typename ArrowType>
        double _slogl(const DataFrame& df) const;

        template<typename ArrowType>
        Array_ptr _sample(int n, const DataFrame& evidence_values, long unsigned int seed) const;
        
        template<typename ArrowType>
        Array_ptr _sample_multivariate(int n, const DataFrame& evidence_values, long unsigned int seed) const;

        template<typename ArrowType>
        VectorXi _sample_indices_multivariate(Matrix<typename ArrowType::c_type, Dynamic, 1>& random_prob,
                                              const DataFrame& evidence_values,
                                              int n) const;

        template<typename ArrowType, typename KDEType>
        cl::Buffer _sample_indices_from_weights(cl::Buffer& random_prob, cl::Buffer& test_buffer, int n) const;

        template<typename ArrowType>
        VectorXd _cdf(const DataFrame& df) const;

        template<typename ArrowType>
        cl::Buffer _cdf_univariate(cl::Buffer& test_buffer, int m) const;

        template<typename ArrowType, typename KDEType>
        cl::Buffer _cdf_multivariate(cl::Buffer& variable_test_buffer, cl::Buffer& evidence_test_buffer, int m) const;

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
        m_joint.fit(df);
        N = m_joint.num_instances();

        if (!m_evidence.empty()) {
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
    VectorXd CKDE::_logl(const DataFrame& df) const {
        using CType = typename ArrowType::c_type;
        using VectorType = Matrix<CType, Dynamic, 1>;

        auto logl_joint = m_joint.logl_buffer<ArrowType>(df);
        
        auto combined_bitmap = df.combined_bitmap(m_variables);
        auto m = df->num_rows();
        if (combined_bitmap)
            m = util::bit_util::non_null_count(combined_bitmap, df->num_rows());
        
        auto& opencl = OpenCLConfig::get();
        if (!m_evidence.empty()) {
            cl::Buffer logl_marg;
            if (combined_bitmap)
                logl_marg = m_marg.logl_buffer<ArrowType>(df, combined_bitmap);
            else
                logl_marg = m_marg.logl_buffer<ArrowType>(df);

            auto k_substract = opencl.kernel(OpenCL_kernel_traits<ArrowType>::substract_vectors);
            k_substract.setArg(0, logl_joint);
            k_substract.setArg(1, logl_marg);
            auto& queue = opencl.queue();
            queue.enqueueNDRangeKernel(k_substract, cl::NullRange,  cl::NDRange(m), cl::NullRange);
        }

        if (combined_bitmap) {
            VectorType read_data(m);
            auto bitmap_data = combined_bitmap->data();

            opencl.read_from_buffer(read_data.data(), logl_joint, m);

            VectorXd res(df->num_rows());

            for (int i = 0, k = 0; i < df->num_rows(); ++i) {
                if(arrow::BitUtil::GetBit(bitmap_data, i)) {
                    res(i) = static_cast<double>(read_data[k++]);
                } else {
                    res(i) = util::nan<double>;
                }
            }

            return res;
        } else {
            VectorType read_data(df->num_rows());
            opencl.read_from_buffer(read_data.data(), logl_joint, df->num_rows());
            if constexpr (!std::is_same_v<CType, double>)
                return read_data.template cast<double>();
            else
                return read_data;
        }
    }

    template<typename ArrowType>
    double CKDE::_slogl(const DataFrame& df) const {
        using CType = typename ArrowType::c_type;

        auto logl_joint = m_joint.logl_buffer<ArrowType>(df);

        auto combined_bitmap = df.combined_bitmap(m_variables);
        auto m = df->num_rows();
        if (combined_bitmap)
            m = util::bit_util::non_null_count(combined_bitmap, df->num_rows());

        auto& opencl = OpenCLConfig::get();
        if(!m_evidence.empty()) {
            cl::Buffer logl_marg;
            if (combined_bitmap)
                logl_marg = m_marg.logl_buffer<ArrowType>(df, combined_bitmap);
            else
                logl_marg = m_marg.logl_buffer<ArrowType>(df);
            
            auto k_substract = opencl.kernel(OpenCL_kernel_traits<ArrowType>::substract_vectors);
            k_substract.setArg(0, logl_joint);
            k_substract.setArg(1, logl_marg);
            auto& queue = opencl.queue();
            queue.enqueueNDRangeKernel(k_substract, cl::NullRange,  cl::NDRange(m), cl::NullRange);
        }

        auto buffer_sum = opencl.sum1d<ArrowType>(logl_joint, m);

        CType result = 0;
        opencl.read_from_buffer(&result, buffer_sum, 1);
        return static_cast<double>(result);
    }

    template<typename ArrowType>
    Array_ptr CKDE::_sample(int n, const DataFrame& evidence_values, long unsigned int seed) const {
        using CType = typename ArrowType::c_type;
        using VectorType = Matrix<CType, Dynamic, 1>;
        using MatrixType = Matrix<CType, Dynamic, Dynamic>;
        
        if (m_evidence.empty()) {
            arrow::NumericBuilder<ArrowType> builder;
            builder.Resize(n);
            std::mt19937 rng{seed};
            std::uniform_int_distribution<> uniform(0, N-1);
            
            std::normal_distribution<CType> normal(0, std::sqrt(m_joint.bandwidth()(0,0)));
            VectorType training_data(N);
            const auto& training_buffer = m_joint.training_buffer();
            auto& opencl = OpenCLConfig::get();
            opencl.read_from_buffer(training_data.data(), training_buffer, N);
            
            for (auto i = 0; i < n; ++i) {
                auto index = uniform(rng);
                builder.UnsafeAppend(training_data(index) + normal(rng));
            }

            Array_ptr out;
            auto status = builder.Finish(&out);
            if (!status.ok()) {
                throw std::runtime_error("New array could not be created. Error status: " + status.ToString());
            }

            return out;
        } else {
            return _sample_multivariate<ArrowType>(n, evidence_values, seed);
        }
    }

    template<typename ArrowType>
    Array_ptr CKDE::_sample_multivariate(int n, const DataFrame& evidence_values, long unsigned int seed) const {
        using CType = typename ArrowType::c_type;
        using ArrowArrayType = typename arrow::TypeTraits<ArrowType>::ArrayType;
        using VectorType = Matrix<CType, Dynamic, 1>;
        using MatrixType = Matrix<CType, Dynamic, Dynamic>;

        VectorType random_prob(n);
        std::mt19937 rng{seed};
        std::uniform_real_distribution<CType> uniform(0, 1);
        for(auto i = 0; i < n; ++i) {
            random_prob(i) = uniform(rng);
        }

        VectorXi sample_indices = _sample_indices_multivariate<ArrowType>(random_prob, evidence_values, n);

        const auto& bandwidth = m_joint.bandwidth();
        const auto& marg_bandwidth = m_marg.bandwidth();
        
        auto cholesky = marg_bandwidth.llt();
        auto matrixL = cholesky.matrixL();

        auto d = m_evidence.size();
        MatrixXd inverseL = MatrixXd::Identity(d, d);

        // Solves and saves the result in inverseL
        matrixL.solveInPlace(inverseL);
        auto R = inverseL * bandwidth.bottomLeftCorner(d, 1);
        auto cond_var = bandwidth(0,0) - R.squaredNorm();
        auto transform = (R.transpose() * inverseL).template cast<CType>();

        MatrixType training_dataset(N, m_variables.size());
        auto& opencl = OpenCLConfig::get();
        opencl.read_from_buffer(training_dataset.data(), m_joint.training_buffer(), N*m_variables.size());

        MatrixType evidence_substract(n, m_evidence.size());
        for (auto j = 0; j < m_evidence.size(); ++j) {
            auto evidence = evidence_values->GetColumnByName(m_evidence[j]);
            auto dwn_evidence = std::static_pointer_cast<ArrowArrayType>(evidence);
            auto raw_values = dwn_evidence->raw_values();
            for (auto i = 0; i < n; ++i) {
                evidence_substract(i, j) = raw_values[i] - training_dataset(sample_indices(i), j+1);
            }
        }

        auto cond_mean = (evidence_substract*transform).eval();

        std::normal_distribution<CType> normal(0, std::sqrt(cond_var));
        arrow::NumericBuilder<ArrowType> builder;
        builder.Resize(n);

        for (auto i = 0; i < n; ++i) {
            cond_mean(i) += training_dataset(sample_indices(i), 0) + normal(rng);
        }

        builder.AppendValues(cond_mean.data(), n);

        Array_ptr out;
        auto status = builder.Finish(&out);
        if (!status.ok()) {
            throw std::runtime_error("New array could not be created. Error status: " + status.ToString());
        }

        return out;
    }

    template<typename ArrowType>
    VectorXi CKDE::_sample_indices_multivariate(Matrix<typename ArrowType::c_type, Dynamic, 1>& random_prob,
                                                const DataFrame& evidence_values,
                                                int n) const {
        using CType = typename ArrowType::c_type;
        using ArrowArray = typename arrow::TypeTraits<ArrowType>::ArrayType;
        using MatrixType = Matrix<CType, Dynamic, Dynamic>;

        MatrixType test_matrix(n, m_evidence.size());

        for (auto i = 0; i < m_evidence.size(); ++i) {
            auto evidence = evidence_values->GetColumnByName(m_evidence[i]);

            auto dwn_evidence = std::static_pointer_cast<ArrowArray>(evidence);
            auto raw_evidence = dwn_evidence->raw_values();

            std::memcpy(test_matrix.data() + i*n, raw_evidence, sizeof(CType)*n);
        }

        auto& opencl = OpenCLConfig::get();
        auto test_buffer = opencl.copy_to_buffer(test_matrix.data(), n*m_evidence.size());
        auto buff_random_prob = opencl.copy_to_buffer(random_prob.data(), n);

        cl::Buffer indices_buffer;
        if (m_evidence.size() == 1)
            indices_buffer = _sample_indices_from_weights<ArrowType, UnivariateKDE>(buff_random_prob, test_buffer, n);
        else
            indices_buffer = _sample_indices_from_weights<ArrowType, MultivariateKDE>(buff_random_prob, test_buffer, n);

        VectorXi res(n);
        opencl.read_from_buffer(res.data(), indices_buffer, n);
        return res;
    }

    template<typename ArrowType, typename KDEType>
    cl::Buffer CKDE::_sample_indices_from_weights(cl::Buffer& random_prob,
                                                  cl::Buffer& test_buffer,
                                                  int n) const {
        using CType = typename ArrowType::c_type;

        auto& opencl = OpenCLConfig::get();
        auto res = opencl.new_buffer<int>(n);
        opencl.fill_buffer<int>(res, N-1, n);

        auto [mat_logls, allocated_m] = allocate_mat<ArrowType>(N);
        auto iterations = static_cast<int>(std::ceil(static_cast<double>(n) / static_cast<double>(allocated_m)));

        cl::Buffer tmp_mat_buffer;
        if constexpr(std::is_same_v<KDEType, MultivariateKDE>) {
            if (N > allocated_m)
                tmp_mat_buffer = opencl.new_buffer<CType>(N*m_evidence.size());
            else
                tmp_mat_buffer = opencl.new_buffer<CType>(allocated_m*m_evidence.size());
        }

        auto k_exp = opencl.kernel(OpenCL_kernel_traits<ArrowType>::exp_elementwise);
        k_exp.setArg(0, mat_logls);

        auto k_normalize_accum_sumexp = opencl.kernel(OpenCL_kernel_traits<ArrowType>::normalize_accum_sum_mat_cols);
        k_normalize_accum_sumexp.setArg(0, mat_logls);
        k_normalize_accum_sumexp.setArg(1, static_cast<unsigned int>(N));

        auto k_find_random_indices = opencl.kernel(OpenCL_kernel_traits<ArrowType>::find_random_indices);
        k_find_random_indices.setArg(0, mat_logls);
        k_find_random_indices.setArg(1, static_cast<unsigned int>(N));
        k_find_random_indices.setArg(3, random_prob);
        k_find_random_indices.setArg(4, res);

        for (auto i = 0; i < (iterations-1); ++i) {
            KDEType::template execute_logl_mat<ArrowType>(m_marg.training_buffer(), N, test_buffer, n, 
                                                        i*allocated_m, allocated_m, m_evidence.size(), 
                                                        m_marg.cholesky_buffer(), m_marg.lognorm_const(), 
                                                        tmp_mat_buffer, mat_logls);

            opencl.queue().enqueueNDRangeKernel(k_exp, cl::NullRange, cl::NDRange(N*allocated_m), cl::NullRange);
            
            auto total_sum = opencl.accum_sum_cols<ArrowType>(mat_logls, N, allocated_m);
            
            k_normalize_accum_sumexp.setArg(2, total_sum);
            opencl.queue().enqueueNDRangeKernel(k_normalize_accum_sumexp, 
                                                cl::NullRange, 
                                                cl::NDRange(N-1, allocated_m), 
                                                cl::NullRange);
        
            k_find_random_indices.setArg(2, static_cast<unsigned int>(i*allocated_m));
            opencl.queue().enqueueNDRangeKernel(k_find_random_indices, cl::NullRange, cl::NDRange(N-1, allocated_m), cl::NullRange);
        }
        auto offset = (iterations - 1)*allocated_m;
        auto remaining_m = n - offset;
        KDEType::template execute_logl_mat<ArrowType>(m_marg.training_buffer(), N, test_buffer, n, 
                                                    offset, remaining_m, m_evidence.size(), 
                                                    m_marg.cholesky_buffer(), m_marg.lognorm_const(), 
                                                    tmp_mat_buffer, mat_logls);
        
        opencl.queue().enqueueNDRangeKernel(k_exp, cl::NullRange, cl::NDRange(N*remaining_m), cl::NullRange);

        auto total_sum = opencl.accum_sum_cols<ArrowType>(mat_logls, N, remaining_m);
        
        k_normalize_accum_sumexp.setArg(2, total_sum);
        opencl.queue().enqueueNDRangeKernel(k_normalize_accum_sumexp, 
                                        cl::NullRange, 
                                        cl::NDRange(N-1, remaining_m), 
                                        cl::NullRange
                                    );

        k_find_random_indices.setArg(2, static_cast<unsigned int>(offset));
        opencl.queue().enqueueNDRangeKernel(k_find_random_indices, cl::NullRange, cl::NDRange(N-1, remaining_m), cl::NullRange);

        return res;
    }

    template<typename ArrowType>
    VectorXd CKDE::_cdf(const DataFrame& df) const {
        using CType = typename ArrowType::c_type;
        using VectorType = Matrix<CType, Dynamic, 1>;
        auto& opencl = OpenCLConfig::get();

        cl::Buffer res_buffer;
        auto test_matrix = df.to_eigen<false, ArrowType>(m_variables);
        auto m = test_matrix->rows();

        auto test_buffer = opencl.copy_to_buffer(test_matrix->data(), m);
        if (m_evidence.empty()) {
            res_buffer = _cdf_univariate<ArrowType>(test_buffer, m);
        } else {
            auto evidence_test_buffer = opencl.copy_to_buffer(test_matrix->data()+m, m*m_evidence.size());
            if (m_evidence.size() == 1) {
                res_buffer = _cdf_multivariate<ArrowType, UnivariateKDE>(test_buffer, evidence_test_buffer, m);
            }
            else {
                res_buffer = _cdf_multivariate<ArrowType, MultivariateKDE>(test_buffer, evidence_test_buffer, m);
            }
        }

        if (df.null_count(m_variables) == 0) {
            VectorType read_data(df->num_rows());
            opencl.read_from_buffer(read_data.data(), res_buffer, df->num_rows());
            if constexpr (!std::is_same_v<CType, double>)
                return read_data.template cast<double>();
            else
                return read_data;
        } else {
            auto valid = df.valid_rows(m_variables);
            VectorType read_data(valid);
            auto bitmap = df.combined_bitmap(m_variables);
            auto bitmap_data = bitmap->data();

            opencl.read_from_buffer(read_data.data(), res_buffer, valid);

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
    cl::Buffer CKDE::_cdf_univariate(cl::Buffer& test_buffer, int m) const {
        using CType = typename ArrowType::c_type;
        auto& opencl = OpenCLConfig::get();
        auto res = opencl.new_buffer<CType>(m);

        auto [mu, allocated_m] = allocate_mat<ArrowType>(N);
        auto iterations = std::ceil(static_cast<double>(m) / static_cast<double>(allocated_m));

        auto reduc_buffers = opencl.create_reduction_mat_buffers<ArrowType>(N, allocated_m);

        auto k_cdf = opencl.kernel(OpenCL_kernel_traits<ArrowType>::univariate_normal_cdf);
        k_cdf.setArg(0, m_joint.training_buffer());
        k_cdf.setArg(1, static_cast<unsigned int>(N));
        k_cdf.setArg(2, test_buffer);
        k_cdf.setArg(4, static_cast<CType>(1.0 / m_joint.bandwidth()(0,0)));
        k_cdf.setArg(5, static_cast<CType>(1.0 / N));
        k_cdf.setArg(6, mu);

        for (auto i = 0; i < (iterations-1); ++i) {
            k_cdf.setArg(3, static_cast<unsigned int>(i*allocated_m));
            opencl.queue().enqueueNDRangeKernel(k_cdf, cl::NullRange, cl::NDRange(N*allocated_m), cl::NullRange);
            opencl.sum_cols_offset<ArrowType>(mu, N, allocated_m, res, i*allocated_m, reduc_buffers);
        }

        auto offset = (iterations - 1)*allocated_m;
        auto remaining_m = m - offset;

        k_cdf.setArg(3, static_cast<unsigned int>(offset));
        opencl.queue().enqueueNDRangeKernel(k_cdf, cl::NullRange, cl::NDRange(N*remaining_m), cl::NullRange);

        opencl.sum_cols_offset<ArrowType>(mu, N, remaining_m, res, offset, reduc_buffers);

        return res;
    }

    template<typename ArrowType, typename KDEType>
    cl::Buffer CKDE::_cdf_multivariate(cl::Buffer& variable_test_buffer, cl::Buffer& evidence_test_buffer, int m) const {
        using CType = typename ArrowType::c_type;

        const auto& bandwidth = m_joint.bandwidth();
        const auto& marg_bandwidth = m_marg.bandwidth();
        
        auto cholesky = marg_bandwidth.llt();
        auto matrixL = cholesky.matrixL();

        auto d = m_evidence.size();
        MatrixXd inverseL = MatrixXd::Identity(d, d);

        // Solves and saves the result in inverseL
        matrixL.solveInPlace(inverseL);
        auto R = inverseL * bandwidth.bottomLeftCorner(d, 1);
        auto cond_var = bandwidth(0,0) - R.squaredNorm();
        auto transform = (R.transpose() * inverseL).template cast<CType>().eval();
        
        auto& opencl = OpenCLConfig::get();
        auto transform_buffer = opencl.copy_to_buffer(transform.data(), m_evidence.size());

        auto res = opencl.new_buffer<CType>(m);

        auto [mu, allocated_m] = allocate_mat<ArrowType>(N);
        auto W = opencl.new_buffer<CType>(N*allocated_m) ;
        auto sum_W = opencl.new_buffer<CType>(allocated_m);

        auto iterations = static_cast<int>(std::ceil(static_cast<double>(m) / static_cast<double>(allocated_m)));

        cl::Buffer tmp_mat_buffer;
        if constexpr(std::is_same_v<KDEType, MultivariateKDE>) {
            if (N > allocated_m)
                tmp_mat_buffer = opencl.new_buffer<CType>(N*m_evidence.size());
            else
                tmp_mat_buffer = opencl.new_buffer<CType>(allocated_m*m_evidence.size());
        }

        auto reduc_buffers = opencl.create_reduction_mat_buffers<ArrowType>(N, allocated_m);

        auto k_exp = opencl.kernel(OpenCL_kernel_traits<ArrowType>::exp_elementwise);
        k_exp.setArg(0, W);

        auto k_normal_cdf = opencl.kernel(OpenCL_kernel_traits<ArrowType>::normal_cdf);
        k_normal_cdf.setArg(0, mu);
        k_normal_cdf.setArg(1, static_cast<unsigned int>(N));
        k_normal_cdf.setArg(2, variable_test_buffer);
        k_normal_cdf.setArg(4, static_cast<CType>(1.0 / cond_var));

        auto k_product = opencl.kernel(OpenCL_kernel_traits<ArrowType>::product_elementwise);
        k_product.setArg(0, mu);
        k_product.setArg(1, W);

        auto k_divide = opencl.kernel(OpenCL_kernel_traits<ArrowType>::division_elementwise);
        k_divide.setArg(0, res);
        k_divide.setArg(2, sum_W);

        for (auto i = 0; i < (iterations-1); ++i) {
            // Computes Weigths
            KDEType::template execute_logl_mat<ArrowType>(m_marg.training_buffer(), N, evidence_test_buffer, m, 
                                                        i*allocated_m, allocated_m, m_evidence.size(), 
                                                        m_marg.cholesky_buffer(), m_marg.lognorm_const(), 
                                                        tmp_mat_buffer, W);
            
            opencl.queue().enqueueNDRangeKernel(k_exp, cl::NullRange, cl::NDRange(N*allocated_m), cl::NullRange);
            opencl.sum_cols_offset<ArrowType>(W, N, allocated_m, sum_W, 0, reduc_buffers);

            // Computes conditional mu.
            KDEType::template execute_conditional_means<ArrowType>(m_joint.training_buffer(), 
                                                                    m_marg.training_buffer(),
                                                                    N, evidence_test_buffer, m, 
                                                                    i*allocated_m, allocated_m, 
                                                                    m_evidence.size(), transform_buffer,
                                                                    tmp_mat_buffer, mu);
            k_normal_cdf.setArg(3, static_cast<unsigned int>(i*allocated_m));
            opencl.queue().enqueueNDRangeKernel(k_normal_cdf, cl::NullRange, cl::NDRange(N*allocated_m), cl::NullRange);
            opencl.queue().enqueueNDRangeKernel(k_product, cl::NullRange, cl::NDRange(N*allocated_m), cl::NullRange);
            opencl.sum_cols_offset<ArrowType>(mu, N, allocated_m, res, i*allocated_m, reduc_buffers);
            k_divide.setArg(1, static_cast<unsigned int>(i*allocated_m));
            opencl.queue().enqueueNDRangeKernel(k_divide, cl::NullRange, cl::NDRange(allocated_m), cl::NullRange);
        }

        auto offset = (iterations - 1)*allocated_m;
        auto remaining_m = m - offset;
        // Computes Weigths
        KDEType::template execute_logl_mat<ArrowType>(m_marg.training_buffer(), N, evidence_test_buffer, m, 
                                                    offset, remaining_m, m_evidence.size(), 
                                                    m_marg.cholesky_buffer(), m_marg.lognorm_const(), 
                                                    tmp_mat_buffer, W);
        
        opencl.queue().enqueueNDRangeKernel(k_exp, cl::NullRange, cl::NDRange(N*remaining_m), cl::NullRange);
        opencl.sum_cols_offset<ArrowType>(W, N, remaining_m, sum_W, 0, reduc_buffers);

        // Computes conditional mu.
        KDEType::template execute_conditional_means<ArrowType>(m_joint.training_buffer(), 
                                                                m_marg.training_buffer(),
                                                                N, evidence_test_buffer, m, 
                                                                offset, remaining_m, 
                                                                m_evidence.size(), transform_buffer,
                                                                tmp_mat_buffer, mu);

        k_normal_cdf.setArg(3, static_cast<unsigned int>(offset));
        opencl.queue().enqueueNDRangeKernel(k_normal_cdf, cl::NullRange, cl::NDRange(N*remaining_m), cl::NullRange);
        opencl.queue().enqueueNDRangeKernel(k_product, cl::NullRange, cl::NDRange(N*remaining_m), cl::NullRange);
        opencl.sum_cols_offset<ArrowType>(mu, N, remaining_m, res, offset, reduc_buffers);
        k_divide.setArg(1, static_cast<unsigned int>(offset));
        opencl.queue().enqueueNDRangeKernel(k_divide, cl::NullRange, cl::NDRange(remaining_m), cl::NullRange);

        return res;
    }
}

#endif //PGM_DATASET_CKDE_HPP