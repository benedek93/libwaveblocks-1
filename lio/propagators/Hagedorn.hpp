#include <vector>
#include <iostream>
#include "types.hpp"
#include "matrixPotentials/bases.hpp"
#include "matrixPotentials/potentials.hpp"
#include "hawp_commons.hpp"
#include "inhomogeneous_inner_product.hpp"
#include "homogeneous_inner_product.hpp"
#include "tensor_product_qr.hpp"
#include "gauss_hermite_qr.hpp"



namespace waveblocks
{

  namespace propagators
  {
    template <int N, int D, class MultiIndex, class TQR>
    struct Hagedorn {
      static void propagate( waveblocks::InhomogeneousHaWp<D,MultiIndex> &packet,
                      const real_t &delta_t,
                      const InhomogenousMatrixPotential<N, D> &V,
                      CVector<N> &S
      ) {
        
        // 1.
        int i = 0;
        for (auto& component : packet.components()) {
          
          auto& params = component.parameters();
          params.q += 0.5 * delta_t * params.p;
          params.Q +=  0.5 * delta_t * params.P;
          S[i] += 0.25 * delta_t * params.p.dot(params.p);
        // 2.
          
          // leading levels
          const auto& leading_level_taylor = V.get_leading_level().taylor_at(complex_t(1,0) * params.q);
          params.p -= delta_t*std::get<1>(leading_level_taylor).real();
          params.P -= delta_t*std::get<2>(leading_level_taylor);
          S[i++] -= delta_t*std::get<0>(leading_level_taylor);
        }
        // 3.
        // Set up quadrature
        
        typename TQR::NodeMatrix nodes;
        typename TQR::WeightVector weights;
        waveblocks::InhomogeneousInnerProduct<D, MultiIndex, TQR> ip;

        
        CMatrix<Eigen::Dynamic,Eigen::Dynamic> F;
        int size = 0;
        for (auto& component : packet.components()) {
          size += component.coefficients().size();
        }
        F.resize(size,size);
        int i_offset = 0;
        for (int i = 0; i < N; ++i){
          int i_size = packet.component(i).coefficients().size();
          int j_offset = 0;

          for (int j = 0; j < N; ++j) {
            int j_size = packet.component(j).coefficients().size();
            // Set up operator
            auto op =
                [&V, i, j] (const CMatrix<D,Eigen::Dynamic> &nodes, const RMatrix<D,1> &pos) // Do you want real or complex positions??? (You want real...)
            {
                const dim_t n_nodes = nodes.cols();
                CMatrix<1,Eigen::Dynamic> result(1, n_nodes);
                for(int l = 0; l < n_nodes; ++l) {
                  result(0, l) = V.evaluate_local_remainder_at(nodes.template block<D, 1>(0, l), complex_t(1, 0) * pos)(i,j); // SUUUUPER INEFFICIENT. COMPUTE NxN Matrix when we only want one entry...
                }
                return result;
            };

            // Build matrix
            F.block(i_offset,j_offset,i_size,j_size) = ip.build_matrix( packet.component(i), packet.component(j), op );
            j_offset += j_size;
          }
          i_offset += i_size;
        }

        auto M = -delta_t * ( complex_t( 0, 1 ) / packet.eps() ) * F;
        
        // Exponential
        CMatrix<Eigen::Dynamic,Eigen::Dynamic> expM;
        (Eigen::MatrixExponential<CMatrix<Eigen::Dynamic, Eigen::Dynamic> >( M )).compute( expM );
        
        // Put all coefficients into a vector
        CVector<Eigen::Dynamic> coefficients;
        coefficients.resize(size);
        int j_offset = 0;
        for(auto& component : packet.components()) {
          int j_size = component.coefficients().size();
            for (int j = 0; j < j_size; ++j) {
              coefficients[j+j_offset] = component.coefficients()[j];
            }
          j_offset += j_size;
        }
        
        // Compute product
        coefficients = expM * coefficients;
        
        j_offset = 0;
        // Unpack coefficients
        for(auto& component: packet.components()) {
          int j_size = component.coefficients().size();
            for (int j = 0; j < j_size; ++j) {
              component.coefficients()[j] = coefficients[j+j_offset];
            }
          j_offset += j_size;
        }
        
        // 4.
        i = 0;
        for (auto& component : packet.components()) {
          
          auto& params = component.parameters();
          params.q += 0.5 * delta_t * params.p;
          params.Q +=  0.5 * delta_t * params.P;
          S[i++] += 0.25 * delta_t * params.p.dot(params.p);
        }
      }

      static void propagate( waveblocks::HomogeneousHaWp<D,MultiIndex> &packet,
                      const real_t &delta_t,
                      const HomogenousMatrixPotential<N, D> &V,
                      complex_t &S
      ) {
        
        // 1.
        int i = 0;
        
        auto& params = packet.parameters();
        params.q += 0.5 * delta_t * params.p;
        params.Q +=  0.5 * delta_t * params.P;
        S += 0.25 * delta_t * params.p.dot(params.p);
        // 2.
          
        // leading levels
        const auto& leading_level_taylor = V.get_leading_level().taylor_at(complex_t(1,0) * params.q);
        params.p -= delta_t*std::get<1>(leading_level_taylor).real();
        params.P -= delta_t*std::get<2>(leading_level_taylor);
        S -= delta_t*std::get<0>(leading_level_taylor);
        
        // 3.
        // Set up quadrature
        
        typename TQR::NodeMatrix nodes;
        typename TQR::WeightVector weights;
        waveblocks::HomogeneousInnerProduct<D, MultiIndex, TQR> ip;

        
        CMatrix<Eigen::Dynamic,Eigen::Dynamic> F;
        int size = 0;
        for (auto& component : packet.components()) {
          size += component.coefficients().size();
        }
        F.resize(size,size);
        int i_offset = 0;
        for (int i = 0; i < N; ++i){
          int i_size = packet.component(i).coefficients().size();
          int j_offset = 0;

          for (int j = 0; j < N; ++j) {
            int j_size = packet.component(j).coefficients().size();
            // Set up operator
            auto op =
                [&V, i, j] (const CMatrix<D,Eigen::Dynamic> &nodes, const CMatrix<D,1> &pos) // Do you want real or complex positions??? (You want real...)
            {
                const dim_t n_nodes = nodes.cols();
                CMatrix<1,Eigen::Dynamic> result(1, n_nodes);
                for(int l = 0; l < n_nodes; ++l) {
                  result(0, l) = V.evaluate_local_remainder_at(nodes.template block<D, 1>(0, l), pos)(i,j); // SUUUUPER INEFFICIENT. COMPUTE NxN Matrix when we only want one entry...
                }
                return result;
            };

            // Build matrix
            F.block(i_offset,j_offset,i_size,j_size) = ip.build_matrix( packet.component(i), packet.component(j), op );
            j_offset += j_size;
          }
          i_offset += i_size;
        }

        auto M = -delta_t * ( complex_t( 0, 1 ) / packet.eps() ) * F;
        
        // Exponential
        CMatrix<Eigen::Dynamic,Eigen::Dynamic> expM;
        (Eigen::MatrixExponential<CMatrix<Eigen::Dynamic, Eigen::Dynamic> >( M )).compute( expM );
        
        // Put all coefficients into a vector
        CVector<Eigen::Dynamic> coefficients;
        coefficients.resize(size);
        int j_offset = 0;
        for(auto& component : packet.components()) {
          int j_size = component.coefficients().size();
            for (int j = 0; j < j_size; ++j) {
              coefficients[j+j_offset] = component.coefficients()[j];
            }
          j_offset += j_size;
        }
        
        // Compute product
        coefficients = expM * coefficients;
        
        j_offset = 0;
        // Unpack coefficients
        for(auto& component: packet.components()) {
          int j_size = component.coefficients().size();
            for (int j = 0; j < j_size; ++j) {
              component.coefficients()[j] = coefficients[j+j_offset];
            }
          j_offset += j_size;
        }
        
        // 4.
        auto& params = packet.parameters();
        params.q += 0.5 * delta_t * params.p;
        params.Q +=  0.5 * delta_t * params.P;
        S += 0.25 * delta_t * params.p.dot(params.p);
        }
      }
    };

   template <int N class MultiIndex, class TQR>
    struct Hagedorn<N,1,MultiIndex,TQR> {
      static void propagate( waveblocks::InhomogeneousHaWp<1,MultiIndex> &packet,
                      const real_t &delta_t,
                      const InhomogenousMatrixPotential<N, 1> &V,
                      CVector<N> &S
      ) {
        
        // 1.
        int i = 0;
        for (auto& component : packet.components()) {
          
          auto& params = component.parameters();
          params.q[0] += 0.5 * delta_t * params.p[0];
          params.Q[0] +=  0.5 * delta_t * params.P[0];
          S[i] += 0.25 * delta_t * params.p.dot(params.p);
        // 2.
          
          // leading levels
          const auto& leading_level_taylor = V.get_leading_level().taylor_at(complex_t(1,0) * params.q[0]);
          params.p -= delta_t*std::get<1>(leading_level_taylor).real();
          params.P -= delta_t*std::get<2>(leading_level_taylor);
          S[i++] -= delta_t*std::get<0>(leading_level_taylor);
        }
        // 3.
        // Set up quadrature
        
        typename TQR::NodeMatrix nodes;
        typename TQR::WeightVector weights;
        waveblocks::InhomogeneousInnerProduct<D, MultiIndex, TQR> ip;

        
        CMatrix<Eigen::Dynamic,Eigen::Dynamic> F;
        int size = 0;
        for (auto& component : packet.components()) {
          size += component.coefficients().size();
        }
        F.resize(size,size);
        int i_offset = 0;
        for (int i = 0; i < N; ++i){
          int i_size = packet.component(i).coefficients().size();
          int j_offset = 0;

          for (int j = 0; j < N; ++j) {
            int j_size = packet.component(j).coefficients().size();
            // Set up operator
            auto op =
                [&V, i, j] (const CMatrix<1,Eigen::Dynamic> &nodes, const RMatrix<1,1> &pos) // Do you want real or complex positions??? (You want real...)
            {
                const dim_t n_nodes = nodes.cols();
                CMatrix<1,Eigen::Dynamic> result(1, n_nodes);
                for(int l = 0; l < n_nodes; ++l) {
                  result(0, l) = V.evaluate_local_remainder_at(nodes(0, l), complex_t(1, 0) * pos[0])(i,j); // SUUUUPER INEFFICIENT. COMPUTE NxN Matrix when we only want one entry...
                }
                return result;
            };

            // Build matrix
            F.block(i_offset,j_offset,i_size,j_size) = ip.build_matrix( packet.component(i), packet.component(j), op );
            j_offset += j_size;
          }
          i_offset += i_size;
        }

        auto M = -delta_t * ( complex_t( 0, 1 ) / packet.eps() ) * F;
        
        // Exponential
        CMatrix<Eigen::Dynamic,Eigen::Dynamic> expM;
        (Eigen::MatrixExponential<CMatrix<Eigen::Dynamic, Eigen::Dynamic> >( M )).compute( expM );
        
        // Put all coefficients into a vector
        CVector<Eigen::Dynamic> coefficients;
        coefficients.resize(size);
        int j_offset = 0;
        for(auto& component : packet.components()) {
          int j_size = component.coefficients().size();
            for (int j = 0; j < j_size; ++j) {
              coefficients[j+j_offset] = component.coefficients()[j];
            }
          j_offset += j_size;
        }
        
        // Compute product
        coefficients = expM * coefficients;
        
        j_offset = 0;
        // Unpack coefficients
        for(auto& component: packet.components()) {
          int j_size = component.coefficients().size();
            for (int j = 0; j < j_size; ++j) {
              component.coefficients()[j] = coefficients[j+j_offset];
            }
          j_offset += j_size;
        }
        
        // 4.
        i = 0;
        for (auto& component : packet.components()) {
          
          auto& params = component.parameters();
          params.q[0] += 0.5 * delta_t * params.p[0];
          params.Q[0] +=  0.5 * delta_t * params.P[0];
          S[i++] += 0.25 * delta_t * params.p.dot(params.p);
        }
      }

      static void propagate( waveblocks::HomogeneousHaWp<1,MultiIndex> &packet,
                      const real_t &delta_t,
                      const HomogenousMatrixPotential<N, 1> &V,
                      complex_t &S
      ) {
        
        // 1.
        int i = 0;
        
        auto& params = packet.parameters();
        params.q[0] += 0.5 * delta_t * params.p[0];
        params.Q[0] +=  0.5 * delta_t * params.P[0];
        S += 0.25 * delta_t * params.p.dot(params.p);
        // 2.
          
        // leading levels
        const auto& leading_level_taylor = V.get_leading_level().taylor_at(complex_t(1,0) * params.q[0]);
        params.p[0] -= delta_t*std::get<1>(leading_level_taylor).real();
        params.P[0] -= delta_t*std::get<2>(leading_level_taylor);
        S -= delta_t*std::get<0>(leading_level_taylor);
        
        // 3.
        // Set up quadrature
        
        typename TQR::NodeMatrix nodes;
        typename TQR::WeightVector weights;
        waveblocks::HomogeneousInnerProduct<D, MultiIndex, TQR> ip;

        
        CMatrix<Eigen::Dynamic,Eigen::Dynamic> F;
        int size = 0;
        for (auto& component : packet.components()) {
          size += component.coefficients().size();
        }
        F.resize(size,size);
        int i_offset = 0;
        for (int i = 0; i < N; ++i){
          int i_size = packet.component(i).coefficients().size();
          int j_offset = 0;

          for (int j = 0; j < N; ++j) {
            int j_size = packet.component(j).coefficients().size();
            // Set up operator
            auto op =
                [&V, i, j] (const CMatrix<1,Eigen::Dynamic> &nodes, const CMatrix<1,1> &pos) // Do you want real or complex positions??? (You want real...)
            {
                const dim_t n_nodes = nodes.cols();
                CMatrix<1,Eigen::Dynamic> result(1, n_nodes);
                for(int l = 0; l < n_nodes; ++l) {
                  result(0, l) = V.evaluate_local_remainder_at(nodes(0, l), pos[0])(i,j); // SUUUUPER INEFFICIENT. COMPUTE NxN Matrix when we only want one entry...
                }
                return result;
            };

            // Build matrix
            F.block(i_offset,j_offset,i_size,j_size) = ip.build_matrix( packet.component(i), packet.component(j), op );
            j_offset += j_size;
          }
          i_offset += i_size;
        }

        auto M = -delta_t * ( complex_t( 0, 1 ) / packet.eps() ) * F;
        
        // Exponential
        CMatrix<Eigen::Dynamic,Eigen::Dynamic> expM;
        (Eigen::MatrixExponential<CMatrix<Eigen::Dynamic, Eigen::Dynamic> >( M )).compute( expM );
        
        // Put all coefficients into a vector
        CVector<Eigen::Dynamic> coefficients;
        coefficients.resize(size);
        int j_offset = 0;
        for(auto& component : packet.components()) {
          int j_size = component.coefficients().size();
            for (int j = 0; j < j_size; ++j) {
              coefficients[j+j_offset] = component.coefficients()[j];
            }
          j_offset += j_size;
        }
        
        // Compute product
        coefficients = expM * coefficients;
        
        j_offset = 0;
        // Unpack coefficients
        for(auto& component: packet.components()) {
          int j_size = component.coefficients().size();
            for (int j = 0; j < j_size; ++j) {
              component.coefficients()[j] = coefficients[j+j_offset];
            }
          j_offset += j_size;
        }
        
        // 4.
        auto& params = packet.parameters();
        params.q[0] += 0.5 * delta_t * params.p[0];
        params.Q[0] +=  0.5 * delta_t * params.P[0];
        S += 0.25 * delta_t * params.p.dot(params.p);
        }
      }
    };

    template<int D, class MultiIndex, class TQR>
    struct Hagedorn<1,D,MultiIndex,TQR>
    {
      static void propagate( waveblocks::ScalarHaWp<D,MultiIndex> &packet,
                    const real_t &delta_t,
                    const HomogenousMatrixPotential<1, D> &V,
                    complex_t &S
    ) {

      
      // 1.
        auto& params = packet.parameters();
        params.q += 0.5 * delta_t * params.p;
        params.Q +=  0.5 * delta_t * params.P;
        S += 0.25 * delta_t * params.p.dot(params.p);
      // 2.


        // leading levels
        const auto& leading_level_taylor = V.get_leading_level().taylor_at(complex_t(1,0) * params.q); // QUADRATIC REMAINDER?
        params.p -= delta_t*std::get<1>(leading_level_taylor).real();
        params.P -= delta_t*std::get<2>(leading_level_taylor);
        S -= delta_t*std::get<0>(leading_level_taylor);

      // 3.
      // Set up quadrature

      typename TQR::NodeMatrix nodes;
      typename TQR::WeightVector weights;
      waveblocks::InhomogeneousInnerProduct<D, MultiIndex, TQR> ip;
    
      
      // Set up operator
      auto op =
          [&V] (const CMatrix<D,Eigen::Dynamic> &nodes, const CMatrix<D,1> &pos)  -> CMatrix<1,Eigen::Dynamic>// Do you want real or complex positions??? (You want real...)
      {
          const dim_t n_nodes = nodes.cols();
          CMatrix<1,Eigen::Dynamic> result(n_nodes);
          for(int l = 0; l < n_nodes; ++l) {
            result(0, l) = V.evaluate_local_remainder_at(nodes.template block<D, 1>(0, l), pos);
          }
          return result;
      };

      // Build matrix
      CMatrix<Eigen::Dynamic,Eigen::Dynamic> F = ip.build_matrix(packet, op);
      auto M = -delta_t * ( complex_t( 0, 1 ) / packet.eps() ) * F;
      
      // Exponential
      CMatrix<Eigen::Dynamic,Eigen::Dynamic> expM;
      (Eigen::MatrixExponential<CMatrix<Eigen::Dynamic, Eigen::Dynamic> >( M )).compute( expM );
      
      // Put all coefficients into a vector
      CVector<Eigen::Dynamic> coefficients;
      int size = packet.coefficients().size();
      coefficients.resize(size);
      for (int j = 0; j < size; ++j) {
        coefficients[j] = packet.coefficients()[j];
      }
      
      // Compute product
      coefficients = expM * coefficients;
      
      // Unpack coefficients
      int j_size = packet.coefficients().size();
      for (int j = 0; j < j_size; ++j) {
        packet.coefficients()[j] = coefficients[j];
      }
      
      // 4.
        
        params.q += 0.5 * delta_t * params.p;
        params.Q +=  0.5 * delta_t * params.P;
        S += 0.25 * delta_t * params.p.dot(params.p);
      }
    };
    template<class MultiIndex, class TQR>
    struct Hagedorn<1,1,MultiIndex,TQR>
    {
      static void propagate( waveblocks::ScalarHaWp<1,MultiIndex> &packet,
                    const real_t &delta_t,
                    const HomogenousMatrixPotential<1, 1> &V,
                    complex_t &S
    ) {

      
      // 1.
        auto& params = packet.parameters();
        params.q[0] += 0.5 * delta_t * params.p[0];
        params.Q[0] +=  0.5 * delta_t * params.P[0];
        S += 0.25 * delta_t * params.p.dot(params.p);
      // 2.


        // leading levels
        const auto& leading_level_taylor = V.get_leading_level().taylor_at(complex_t(1,0) * params.q[0]]); // QUADRATIC REMAINDER?
        params.p[0] -= delta_t*std::get<1>(leading_level_taylor).real();
        params.P[0] -= delta_t*std::get<2>(leading_level_taylor);
        S -= delta_t*std::get<0>(leading_level_taylor);

      // 3.
      // Set up quadrature

      typename TQR::NodeMatrix nodes;
      typename TQR::WeightVector weights;
      waveblocks::InhomogeneousInnerProduct<D, MultiIndex, TQR> ip;
    
      
      // Set up operator
      auto op =
          [&V] (const CMatrix<1,Eigen::Dynamic> &nodes, const CMatrix<1,1> &pos)  -> CMatrix<1,Eigen::Dynamic>// Do you want real or complex positions??? (You want real...)
      {
          const dim_t n_nodes = nodes.cols();
          CMatrix<1,Eigen::Dynamic> result(n_nodes);
          for(int l = 0; l < n_nodes; ++l) {
            result(0, l) = V.evaluate_local_remainder_at(nodes(0, l), pos[0]]);
          }
          return result;
      };

      // Build matrix
      CMatrix<Eigen::Dynamic,Eigen::Dynamic> F = ip.build_matrix(packet, op);
      auto M = -delta_t * ( complex_t( 0, 1 ) / packet.eps() ) * F;
      
      // Exponential
      CMatrix<Eigen::Dynamic,Eigen::Dynamic> expM;
      (Eigen::MatrixExponential<CMatrix<Eigen::Dynamic, Eigen::Dynamic> >( M )).compute( expM );
      
      // Put all coefficients into a vector
      CVector<Eigen::Dynamic> coefficients;
      int size = packet.coefficients().size();
      coefficients.resize(size);
      for (int j = 0; j < size; ++j) {
        coefficients[j] = packet.coefficients()[j];
      }
      
      // Compute product
      coefficients = expM * coefficients;
      
      // Unpack coefficients
      int j_size = packet.coefficients().size();
      for (int j = 0; j < j_size; ++j) {
        packet.coefficients()[j] = coefficients[j];
      }
      
      // 4.
        
        params.q[0] += 0.5 * delta_t * params.p[0];
        params.Q[0] +=  0.5 * delta_t * params.P[0];
        S += 0.25 * delta_t * params.p.dot(params.p);
      }
    };
  }
}