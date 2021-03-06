#include <cctbx/boost_python/flex_fwd.h>

#include <boost/python/class.hpp>
#include <boost/python/def.hpp>
#include <boost/python/args.hpp>
#include <boost/python/return_value_policy.hpp>
#include <boost/python/return_by_value.hpp>
#include <cctbx/xray/minimization.h>

namespace cctbx { namespace xray { namespace boost_python {

  struct apply_shifts_wrappers
  {
    typedef minimization::apply_shifts<scatterer<>, double> w_t;

    static void
    wrap()
    {
      using namespace boost::python;
      typedef return_value_policy<return_by_value> rbv;
      class_<w_t>("minimization_apply_shifts", no_init)
        .def(init<
          uctbx::unit_cell const&,
          af::const_ref<scatterer<> > const&,
          af::const_ref<double> const& >((
            arg("unit_cell"),
            arg("scatterers"),
            arg("shifts"))))
        .add_property("shifted_scatterers",
          make_getter(&w_t::shifted_scatterers, rbv()))
        .add_property("u_iso_refinable_params",
          make_getter(&w_t::u_iso_refinable_params, rbv()))
      ;
    }
  };

  void wrap_minimization()
  {
    using namespace boost::python;

    def("damp_shifts",
      (void(*)(
        af::const_ref<double> const&,
        af::ref<double>,
        double const&)) minimization::damp_shifts, (
          arg("previous"),
          arg("current"),
          arg("max_value")));

    def("truncate_shifts",
      (void(*)(
        af::ref<double>,
        double const&,
        double const&)) minimization::truncate_shifts, (
          arg("shifts"),
          arg("min_value"),
          arg("max_value")));

    def("minimization_shift_scales",
      (af::shared<double>(*)(
        af::const_ref<scatterer<> > const&,
        std::size_t,
        double const&,
        double const&,
        double const&,
        double const&,
        double const&,
        double const&)) minimization::shift_scales, (
          arg("scatterers"),
          arg("n_parameters"),
          arg("site_cart"),
          arg("u_iso"),
          arg("u_cart"),
          arg("occupancy"),
          arg("fp"),
          arg("fdp")));

    apply_shifts_wrappers::wrap();

    def("minimization_add_gradients",
      (void(*)(
        af::const_ref<scatterer<> > const&,
        af::ref<double> const&,
        af::const_ref<scitbx::vec3<double> > const&,
        af::const_ref<double> const&,
        af::const_ref<scitbx::sym_mat3<double> > const&,
        af::const_ref<double> const&))
          minimization::add_gradients, (
      arg("scatterers"),
      arg("xray_gradients"),
      arg("site_gradients"),
      arg("u_iso_gradients"),
      arg("u_aniso_gradients"),
      arg("occupancy_gradients")));
    def("minimization_extract_site_gradients",
      (af::shared<scitbx::vec3<double> >(*)(
        af::const_ref<scatterer<> > const&,
        af::const_ref<double> const&))
          minimization::extract_site_gradients, (
      arg("scatterers"),
      arg("xray_gradients")));
  }

}}} // namespace cctbx::xray::boost_python
