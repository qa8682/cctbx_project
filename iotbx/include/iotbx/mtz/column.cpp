#include <iotbx/mtz/column.h>

namespace iotbx { namespace mtz {

  column::column(dataset const& mtz_dataset, int i_column)
  :
    mtz_dataset_(mtz_dataset),
    i_column_(i_column)
  {
    CCTBX_ASSERT(i_column >= 0);
    CCTBX_ASSERT(i_column < mtz_dataset.n_columns());
  }

  CMtz::MTZCOL*
  column::ptr() const
  {
    CCTBX_ASSERT(mtz_dataset_.n_columns() > i_column_);
    return CMtz::MtzIcolInSet(mtz_dataset_.ptr(), i_column_);
  }

  std::string
  column::path() const
  {
    boost::shared_ptr<char>
      p(CMtz::MtzColPath(mtz_object().ptr(), ptr()), free);
    return std::string(p.get());
  }

  int
  column::n_valid_values() const
  {
    int result = 0;
    int n_refl = mtz_object().n_reflections();
    for(int i_refl=0;i_refl<n_refl;i_refl++) {
      if (!is_ccp4_nan(i_refl)) result += 1;
    }
    return result;
  }

  af::shared<int>
  column::set_reals(
    af::const_ref<cctbx::miller::index<> > const& miller_indices,
    af::const_ref<double> const& data)
  {
    typedef std::map<cctbx::miller::index<>, int> miller_map_type;
    CCTBX_ASSERT(data.size() == miller_indices.size());
    CMtz::MTZ* p = mtz_object().ptr();
    int nref_at_entry = p->nref;
    if (nref_at_entry == 0) {
      mtz_object().adjust_column_array_sizes(
        static_cast<int>(miller_indices.size()));
    }
    af::shared<int> result((af::reserve(miller_indices.size())));
    miller_map_type miller_map;
    hkl_columns hkl = mtz_object().get_hkl_columns();
    for(int i_refl=0;i_refl<p->nref;i_refl++) {
      cctbx::miller::index<> h = hkl.get_miller_index(i_refl);
      miller_map_type::iterator entry = miller_map.find(h);
      CCTBX_ASSERT(entry == miller_map.end());
      miller_map[h] = i_refl;
    }
    CMtz::MTZCOL* col_ptrs[4];
    for(int i=0;i<3;i++) col_ptrs[i] = hkl[i].ptr();
    col_ptrs[3] = ptr();
    for(std::size_t i_miller=0;i_miller<miller_indices.size();i_miller++) {
      cctbx::miller::index<> const& h = miller_indices[i_miller];
      af::tiny<float, 4> adata;
      for(int i=0;i<3;i++) adata[i] = h[i];
      adata[3] = data[i_miller];
      miller_map_type::iterator entry = miller_map.find(h);
      int iref = p->nref;
      if (entry == miller_map.end()) {
        if (nref_at_entry != 0) {
          mtz_object().adjust_column_array_sizes(iref+1);
        }
        if (!CMtz::ccp4_lwrefl(p, adata.elems, col_ptrs, 4, iref+1)) {
          throw cctbx::error(CCP4::ccp4_strerror(ccp4_errno));
        }
        miller_map[h] = iref;
        result.push_back(iref);
        iref++;
      }
      else {
        if (entry->second >= nref_at_entry) {
          throw cctbx::error("Duplicate entries in miller_indices array.");
        }
        if (!CMtz::ccp4_lwrefl(p, adata.elems, col_ptrs, 4, entry->second+1)) {
          throw cctbx::error(CCP4::ccp4_strerror(ccp4_errno));
        }
        result.push_back(entry->second);
      }
      CCTBX_ASSERT(p->nref == iref);
    }
    return result;
  }

  void
  column::set_reals(
    af::const_ref<int> const& mtz_reflection_indices,
    af::const_ref<double> const& data)
  {
    CCTBX_ASSERT(data.size() == mtz_reflection_indices.size());
    CMtz::MTZ* p = mtz_object().ptr();
    int nref_at_entry = p->nref;
    CCTBX_ASSERT(nref_at_entry > 0);
    CCTBX_ASSERT(mtz_reflection_indices.size() <= nref_at_entry);
    hkl_columns hkl = mtz_object().get_hkl_columns();
    CMtz::MTZCOL* col_ptrs[4];
    for(int i=0;i<3;i++) col_ptrs[i] = hkl[i].ptr();
    col_ptrs[3] = ptr();
    for(std::size_t i_iref=0;i_iref<mtz_reflection_indices.size();i_iref++) {
      int iref = mtz_reflection_indices[i_iref];
      CCTBX_ASSERT(iref < nref_at_entry);
      cctbx::miller::index<> h = hkl.get_miller_index(iref);
      af::tiny<float, 4> adata;
      for(int i=0;i<3;i++) adata[i] = h[i];
      adata[3] = data[i_iref];
      if (!CMtz::ccp4_lwrefl(p, adata.elems, col_ptrs, 4, iref+1)) {
        throw cctbx::error(CCP4::ccp4_strerror(ccp4_errno));
      }
    }
    CCTBX_ASSERT(p->nref == nref_at_entry);
  }

}} // namespace iotbx::mtz
