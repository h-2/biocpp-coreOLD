// -----------------------------------------------------------------------------------------------------
// Copyright (c) 2006-2019, Knut Reinert & Freie Universität Berlin
// Copyright (c) 2016-2019, Knut Reinert & MPI für molekulare Genetik
// This file may be used, modified and/or redistributed under the terms of the 3-clause BSD-License
// shipped with this file and also available at: https://github.com/seqan/seqan3/blob/master/LICENSE.md
// -----------------------------------------------------------------------------------------------------

/*!\file
 * \brief Provides seqan3::detail::alignment_matrix_policy.
 * \author Rene Rahn <rene.rahn AT fu-berlin.de>
 */

#pragma once

#include <seqan3/alignment/band/static_band.hpp>
#include <seqan3/alignment/pairwise/detail/alignment_algorithm_cache.hpp>
#include <seqan3/core/type_traits/basic.hpp>
#include <seqan3/range/views/slice.hpp>

namespace seqan3::detail
{

/*!\brief Manages the alignment and score matrix.
 * \ingroup alignment_policy
 *
 * \tparam alignment_algorithm_t The derived type (seqan3::detail::alignment_algorithm) to be augmented with this
 *                               CRTP-policy.
 * \tparam score_matrix_t The type of the alignment score matrix.
 * \tparam trace_matrix_t The type of the alignment trace matrix.
 *
 * \details
 *
 * This policy is used to manage the score and trace matrix of the alignment algorithm. On invocation of an alignment
 * instance the necessary memory is allocated and the corresponding matrix iterators are initialised. These
 * iterators are used as a global state within this particular alignment instance and are accessed from the alignment
 * algorithm.
 *
 * \remarks The template parameters of this CRTP-policy are selected in the
 *          seqan3::detail::alignment_configurator::select_matrix_policy when selecting the alignment for the given
 *          configuration.
 */
template <typename alignment_algorithm_t, typename score_matrix_t, typename trace_matrix_t>
class alignment_matrix_policy
{
private:
    //!\brief Allow alignment algorithm to instantiate this crtp base class.
    friend alignment_algorithm_t;

    /*!\name Constructors, destructor and assignment
     * \{
     */
    constexpr alignment_matrix_policy() = default; //!< Defaulted.
    constexpr alignment_matrix_policy(alignment_matrix_policy const &) = default; //!< Defaulted.
    constexpr alignment_matrix_policy(alignment_matrix_policy &&) = default; //!< Defaulted.
    constexpr alignment_matrix_policy & operator=(alignment_matrix_policy const &) = default; //!< Defaulted.
    constexpr alignment_matrix_policy & operator=(alignment_matrix_policy &&) = default; //!< Defaulted.
    ~alignment_matrix_policy() = default; //!< Defaulted.
    //!}

    /*!\brief Allocates the memory of the underlying matrices.
     * \tparam fst_range_t The type of the first sequence to align; must model std::forward_ranges.
     * \tparam snd_range_t The type of the second sequence to align; must model std::forward_ranges.
     * \tparam fst_range_t The type of the first sequence to align.
     * \tparam sec_range_t The type of the second sequence to align.
     * \param[in] fst_range The first sequence to align.
     * \param[in] snd_range The second sequence to align.
     *
     * \details
     *
     * Initialises the underlying score and trace matrices and sets the respective matrix iterators to the begin of the
     * corresponding matrix.
     */
    template <typename fst_range_t, typename snd_range_t>
    constexpr void allocate_matrix(fst_range_t && fst_range, snd_range_t && snd_range)
    {
        score_matrix = score_matrix_t{fst_range, snd_range};
        trace_matrix = trace_matrix_t{fst_range, snd_range};

        initialise_matrix_iterator();
    }

    /*!\brief Allocates the memory of the underlying matrices.
     * \tparam fst_range_t The type of the first sequence to align; must model std::forward_ranges.
     * \tparam snd_range_t The type of the second sequence to align; must model std::forward_ranges.
     * \tparam score_t The score type used inside of the alignment algorithm.
     * \param[in] fst_range The first sequence to align.
     * \param[in] snd_range The second sequence to align.
     * \param[in] band The band used to initialise the matrices.
     * \param[in] cache The current cache used by the alignment algorithm.
     *
     * \details
     *
     * Initialises the underlying banded score and trace matrices and sets the respective matrix iterators to the begin
     * of the corresponding matrix. Using the additional band parameter the actual dimensions are reduced according
     * to the matrix implementation. For the banded case, one additional cell per column is stored such that we can read
     * from it without introducing a case distinction inside of the algorithm implementation. However, this cell needs
     * to be properly initialised with an infinity value. To emulate the infinity for integral values we use the
     * smallest representable value and subtract the gap extension score (assumed to be always negative) from it.
     * In the algorithm we never write to this cell and only add the extension costs to the read value. This way we
     * can get the smallest possible value as an infinity.
     */
    template <typename fst_range_t, typename snd_range_t, typename score_t>
    constexpr void allocate_matrix(fst_range_t && fst_range,
                                   snd_range_t && snd_range,
                                   static_band const & band,
                                   alignment_algorithm_cache<score_t> const & cache)
    {
        assert(cache.gap_extension_score <= 0); // We expect it to be always non-positive.

        score_t inf = std::numeric_limits<score_t>::lowest() - cache.gap_extension_score;
        score_matrix = score_matrix_t{fst_range, snd_range, band, inf};
        trace_matrix = trace_matrix_t{fst_range, snd_range, band};

        initialise_matrix_iterator();
    }

    //!\brief Initialises the score and trace matrix iterator after allocating the matrices.
    constexpr void initialise_matrix_iterator() noexcept
    {
        score_matrix_iter = score_matrix.begin();
        trace_matrix_iter = trace_matrix.begin();
    }

    /*!\brief Slices the sequences according to the band parameters.
    * \tparam fst_range_t The type of the first sequence to align; must model std::forward_ranges.
    * \tparam snd_range_t The type of the second sequence to align; must model std::forward_ranges.
    * \param[in] fst_range The first sequence to align.
    * \param[in] snd_range The second sequence to align.
    * \param[in] band The seqan3::static_band used to limit the alignment space.
    *
    * \details
    *
    * If the band does not intersect with the origin or the sink of the matrix the sequences are sliced such that the
    * band starts in the origin and ends in the sink.
    */
    template <typename fst_range_t, typename snd_range_t>
    constexpr auto slice_sequences(fst_range_t & fst_range,
                                   snd_range_t & snd_range,
                                   static_band const & band) const noexcept
    {
        size_t size_fst = std::ranges::distance(std::ranges::begin(fst_range), std::ranges::end(fst_range));
        size_t size_snd = std::ranges::distance(std::ranges::begin(snd_range), std::ranges::end(snd_range));

        auto trim_fst_range = [&] () constexpr
        {
            size_t begin_pos = std::max<std::ptrdiff_t>(band.lower_bound - 1, 0);
            size_t end_pos = std::min<std::ptrdiff_t>(band.upper_bound + size_snd, size_fst);
            return fst_range | views::slice(begin_pos, end_pos);
        };

        auto trim_snd_range = [&] () constexpr
        {
            size_t begin_pos = std::abs(std::min<std::ptrdiff_t>(band.upper_bound + 1, 0));
            size_t end_pos = std::min<std::ptrdiff_t>(size_fst - band.lower_bound, size_snd);
            return snd_range | views::slice(begin_pos, end_pos);
        };

        return std::tuple{trim_fst_range(), trim_snd_range()};
    }

    score_matrix_t score_matrix{}; //!< The scoring matrix.
    trace_matrix_t trace_matrix{}; //!< The trace matrix if needed.

    //!\brief Allow seqan3::detail::find_optimum_policy to access score_matrix_iter.
    template <typename other_alignment_algorithm_t, typename other_traits_type>
    friend class find_optimum_policy;

    typename score_matrix_t::iterator score_matrix_iter{}; //!< The matrix iterator over the score matrix.
    typename trace_matrix_t::iterator trace_matrix_iter{}; //!< The matrix iterator over the trace matrix.
};
}  // namespace seqan3::detail
