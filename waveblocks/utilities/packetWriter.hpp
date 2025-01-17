#pragma once

#include <iostream>

#include "../hdf5/eigen3-hdf5.hpp"
#include "../types.hpp"

#include "to_string.hpp"


namespace waveblocks {
    namespace utilities {
        namespace packetWriter {
            using wavepackets::ScalarHaWp;

            template<class Subtype, class Packet>
            class Abstract {
                std::string file_name;
                H5::H5File file;
            public:
                Abstract(const std::string& file_name) : file_name(file_name), file(file_name, H5F_ACC_TRUNC) {}
                ~Abstract() {file.close();}

                void store_packet(const double& t, const Packet& packet) {
                    static_cast<Subtype*>(this)->store_packet_implementation(t,packet);
                }

                void store_energies(const double& time, const real_t& epot, const real_t& ekin) {
                    static_cast<Subtype*>(this)->store_energies_implementation(time,epot,ekin);
                }

                template<class M>
                void save_matrix(const M& mat, const std::string& matrix_name)
                {
                    EigenHDF5::save(file, matrix_name, mat);
                }


                template<class M>
                void save_matrix(M& mat, const std::string& matrix_name)
                {
                    EigenHDF5::load(file, matrix_name, mat);
                }

                template<class T>
                void save_scalar(const T& x, const std::string& scalar_name) {
                    GMatrix<T,1,1> m; m[0] = x;
                    EigenHDF5::save(file,scalar_name,m);
                }

            };

            template<class Packet>
            struct Standard;

            template<int D, class MultiIndex>
            struct Standard<ScalarHaWp<D,MultiIndex>> : public Abstract<Standard<ScalarHaWp<D,MultiIndex>>,ScalarHaWp<D,MultiIndex>> {
                using Packet = ScalarHaWp<D,MultiIndex>;
                using Super = Abstract<Standard<ScalarHaWp<D,MultiIndex>>,ScalarHaWp<D,MultiIndex>>;
                Standard(const std::string& file_name) : Super(file_name) {}

                void store_packet_implementation(const double& time, const Packet& packet) {

                    std::string t = to_string<double>(time);
                    // coefficients
                    Super::save_matrix(PacketToCoefficients<Packet>::to(packet), "coefficients@" + t);

                    //
                    const auto& params = packet.parameters();
                    Super::save_matrix(params.q(), "q@" + t);
                    Super::save_matrix(params.p(), "p@" + t);
                    Super::save_matrix(params.Q(), "Q@" + t);
                    Super::save_matrix(params.P(), "P@" + t);
                    Super::save_scalar(params.S(), "S@" + t);

                    // eps
                    Super::save_scalar(packet.eps(), "eps@" + t);
                }

                void store_energies_implementation(const double& time, const real_t& epot, const real_t& ekin) {
                    std::string t = to_string<double>(time);
                    Super::save_scalar(epot, "Epot@" + t);
                    Super::save_scalar(ekin, "Ekin@" + t);
                    Super::save_scalar(epot+ekin, "Etot@" + t);
                }
            };
        }
        template<class Packet>
        using PacketWriter = packetWriter::Standard<Packet>;
    }
}
