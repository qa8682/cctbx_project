/* Copyright (c) 2001-2002 The Regents of the University of California
   through E.O. Lawrence Berkeley National Laboratory, subject to
   approval by the U.S. Department of Energy.
   See files COPYRIGHT.txt and LICENSE.txt for further details.

   Revision history:
     2002 Sep: Refactored (rwgk)
     2001 Sep: start port of sglite/sgss.c (R.W. Grosse-Kunstleve)
 */

#ifndef CCTBX_SGTBX_SEMINVARIANT_H
#define CCTBX_SGTBX_SEMINVARIANT_H

#include <cctbx/sgtbx/space_group.h>

namespace cctbx { namespace sgtbx {

  //! Entry in list of structure seminvariant vectors and moduli.
  struct ss_vec_mod
  {
    sg_vec3 v;
    int m;
  };

  //! Structure-seminvariant vector and moduli.
  /*! Implementation of the algorithm published in section 6. of
      <A HREF="http://journals.iucr.org/a/issues/1999/02/02/au0146/"
      ><I>Acta Cryst.</I> 1999, <B>A55</B>:383-395</A>.<br>
      <p>
      Structure seminvariant (s.s.) vectors and moduli are
      a description of "permissible" or "allowed" origin
      shifts. These are important in crystal structure
      determination methods (e.g. direct methods) or for
      comparing crystal structures.
      <p>
      If the origin of the basis for a given group of
      symmetry operations is shifted by an allowed origin
      shift, the symmetry environment of the old and the
      new origin is identical. See International Tables for
      Crystallography Volume B, 2001, chapter 2.2.3. for
      details.
      <p>
      Allowed origin-shifts are also a part of the
      Euclidean normalizer symmetry. See International
      Tables for Crystallography Volume A, 1983, Table
      15.3.2., column "Translations."
      <p>
      See also: space_group_type::addl_generators_of_euclidean_normalizer()
   */
  class structure_seminvariant
  {
    public:
      //! Default constructor.
      /*! Default-constructed instances have 0 vectors and moduli.
          <p>
          Not available in Python.
       */
      structure_seminvariant() {}

      /*! \brief Computes structure-seminvariant vectors and moduli
          for the given space group.
       */
      /*! See class details.
       */
      structure_seminvariant(space_group const& sg);

      //! The computed structure-seminvariant vectors and moduli.
      af::small<ss_vec_mod, 3> const&
      vectors_and_moduli() const { return vec_mod_; }

      //! Shorthand for vectors_and_moduli().size()
      std::size_t
      size() const { return vectors_and_moduli().size(); }

      /*! \brief Tests if the phase angle of the reflection with
          the Miller index h is a structure-seminvariant.
       */
      bool
      is_ss(miller::index<> const& h) const;

      /*! \brief Reduces the Miller index h for establishing the
           <i>primitivity condition</i>.
       */
      /*! The expression (vectors(i) * h) % moduli(i) is computed for
          each of the size() structure-seminvariant vectors and moduli.
          The size() results can be
          used to establish the <i>primitivity condition</i>
          (see International Tables for Crystallography Volume B,
          2001, chapter 2.2.3(h)):
          <p>
          A square (size() x size()) matrix is formed with size()
          reduced Miller indices. The size() reflections define
          the origin uniquely if the determinant of the matrix
          is +1 or -1.
       */
      af::small<int, 3>
      apply_mod(miller::index<> const& h) const;

      //! Refines gridding starting with grid (1,1,1).
      /*! See also: refine_gridding()
       */
      sg_vec3
      gridding() const { return refine_gridding(sg_vec3(1,1,1)); }

      /*! \brief Refines gridding such that each grid point is
          mapped onto another grid point by the seminvariant
          translations.
       */
      template <typename GridTupleType>
      GridTupleType
      refine_gridding(GridTupleType const& grid) const;

      //! Replaces the moduli with the value 0 by the corresponding gridding.
      template <typename DimensionTupleType>
      af::small<ss_vec_mod, 3>
      grid_adapted_moduli(DimensionTupleType const& dim) const;

    private:
      af::small<ss_vec_mod, 3> vec_mod_;
  };

  template <typename GridTupleType>
  GridTupleType
  structure_seminvariant
  ::refine_gridding(GridTupleType const& grid) const
  {
    GridTupleType ref_grid = grid;
    for(std::size_t i_ss=0;i_ss<size();i_ss++) {
      ss_vec_mod const& ss = vec_mod_[i_ss];
      if (ss.m != 0) {
        for(std::size_t i=0;i<3;i++) {
          ref_grid[i] = boost::lcm(ref_grid[i],
                                   norm_denominator(ss.v[i], ss.m));
        }
      }
    }
    return ref_grid;
  }

  template <typename DimensionTupleType>
  af::small<ss_vec_mod, 3>
  structure_seminvariant
  ::grid_adapted_moduli(DimensionTupleType const& dim) const
  {
    af::small<ss_vec_mod, 3> result = vec_mod_;
    for(ss_vec_mod* r = result.begin(); r != result.end(); r++) {
      if (r->m == 0) {
        r->m = 1;
        for(std::size_t i=0;i<3;i++) {
          if (r->v[i] != 0) {
            r->m = boost::lcm(r->m, norm_denominator(r->v[i], dim[i]));
          }
        }
      }
    }
    return result;
  }

}} // namespace cctbx::sgtbx

#endif // CCTBX_SGTBX_SEMINVARIANT_H
