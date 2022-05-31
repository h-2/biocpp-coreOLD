// -----------------------------------------------------------------------------------------------------
// Copyright (c) 2006-2020, Knut Reinert & Freie Universität Berlin
// Copyright (c) 2016-2020, Knut Reinert & MPI für molekulare Genetik
// This file may be used, modified and/or redistributed under the terms of the 3-clause BSD-License
// shipped with this file and also available at: https://github.com/seqan/seqan3/blob/master/LICENSE.md
// -----------------------------------------------------------------------------------------------------

/*!\file
 * \brief Provides an hacky overload for the googletest PrintTo function.
 * \author Svenja Mehringer <svenja.mehringer AT fu-berlin.de>
 */

#pragma once

#include <ios>

#include <bio/meta/debug_stream.hpp>

namespace bio
{

//!\cond DEV
//!\brief Overload for the googletest PrintTo function that always delegates to our debug_stream.
template <typename t>
    requires (!std::input_or_output_iterator<t> && !std::same_as<t, std::default_sentinel_t>)
    // tricks the compiler to consider this as more specialized than googletests generic PrintTo
void PrintTo (t const & v, std::ostream * out)
{
    debug_stream_type my_stream{*out};
    my_stream << v;
}
//!\endcond

} // namespace bio

namespace std
{

//!\brief Overload for the googletest PrintTo function that always delegates to our debug_stream.
using ::bio::PrintTo;

} // namespace bio::detail

namespace bio::detail
{

//!\brief Overload for the googletest PrintTo function that always delegates to our debug_stream.
using ::bio::PrintTo;

} // namespace bio::detail
