#include <boost/python/class.hpp>
#include <boost/python/def.hpp>
#include <boost/python/args.hpp>
#include <mmtbx/ncs/tncs.h>
#include <boost/python/return_value_policy.hpp>
#include <boost/python.hpp>

namespace mmtbx { namespace ncs {
  namespace bp = boost::python;

namespace {
  boost::python::tuple
  getinitargs(pair<> const& self)
  {
    return boost::python::make_tuple(self.r, self.t);
  }

  using namespace boost::python;
  using boost::python::arg;

  template <typename FloatType>
  struct tncs_eps_factor_refinery_wrapper
  {
    typedef tncs_eps_factor_refinery<FloatType> w_t;
    static void wrap() {
      using namespace boost::python;
      class_<w_t>("tncs_eps_factor_refinery", no_init)
        .def(init<
             bp::list const&,
             af::const_ref<double> const&,
             af::const_ref<double> const&,
             af::const_ref<int> const&,
             af::const_ref<double> const&,
             cctbx::sgtbx::space_group,
             af::const_ref<cctbx::miller::index<int> >,
             scitbx::mat3<double>,
             af::shared<scitbx::mat3<double> >
                  >((arg("tncs_pairs"),
                     arg("f_obs"),
                     arg("sigma_f_obs"),
                     arg("rbin"),
                     arg("SigmaN"),
                     arg("space_group"),
                     arg("miller_indices"),
                     arg("fractionalization_matrix"),
                     arg("sym_matrices"))))
        .def("tncs_epsfac", &w_t::tncs_epsfac_result)
        .def("target", &w_t::target_gradient)
        .def("gradient_rhoMN", &w_t::gradient_rhoMN)
        .def("gradient_radius", &w_t::gradient_radius)
        .def("update_pairs", &w_t::update_pairs)
        .def("set_compute_gradients_rho_mn", &w_t::set_compute_gradients_rho_mn)
        .def("set_compute_gradients_radius", &w_t::set_compute_gradients_radius)
      ;
    }
  };

  void init_module()
  {
    tncs_eps_factor_refinery_wrapper<double>::wrap();

    typedef return_value_policy<return_by_value> rbv;
    class_<pair<> >("pair")
      .def(init<scitbx::mat3<double> const&,
                scitbx::vec3<double> const&,
                double,
                double,
                double,
                af::shared<double>,
                int >((
                  arg("r"),
                  arg("t"),
                  arg("radius"),
                  arg("radius_estimate"),
                  arg("fracscat"),
                  arg("rho_mn"),
                  arg("id"))))
      .add_property("r",      make_getter(&pair<>::r,      rbv()))
      .add_property("t",      make_getter(&pair<>::t,      rbv()))
      .add_property("radius", make_getter(&pair<>::radius, rbv()))
      .add_property("radius_estimate", make_getter(&pair<>::radius_estimate, rbv()))
      .add_property("fracscat", make_getter(&pair<>::fracscat, rbv()))
      .add_property("rho_mn", make_getter(&pair<>::rho_mn, rbv()))
      .add_property("id", make_getter(&pair<>::id, rbv()))
      .enable_pickling()
      .def("__getinitargs__", getinitargs)
      .def("set_radius", &pair<>::set_radius)
      .def("set_rhoMN", &pair<>::set_rhoMN)
      .def("set_id", &pair<>::set_id)
    ;

  }

} // namespace <anonymous>
}} // namespace mmtbx::ncs

BOOST_PYTHON_MODULE(mmtbx_ncs_ext)
{
  mmtbx::ncs::init_module();
}
