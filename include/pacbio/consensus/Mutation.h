// Copyright (c) 2011-2016, Pacific Biosciences of California, Inc.
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted (subject to the limitations in the
// disclaimer below) provided that the following conditions are met:
//
//  * Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//
//  * Redistributions in binary form must reproduce the above
//    copyright notice, this list of conditions and the following
//    disclaimer in the documentation and/or other materials provided
//    with the distribution.
//
//  * Neither the name of Pacific Biosciences nor the names of its
//    contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
// NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
// GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY PACIFIC
// BIOSCIENCES AND ITS CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
// WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
// OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL PACIFIC BIOSCIENCES OR ITS
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
// USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
// OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
// SUCH DAMAGE.

#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <ostream>
#include <string>
#include <tuple>
#include <vector>

#include <boost/numeric/conversion/cast.hpp>
#include <boost/optional.hpp>

namespace PacBio {
namespace Consensus {

// fwd decls
class ScoredMutation;

enum struct MutationType : uint8_t
{
    DELETION,
    INSERTION,
    SUBSTITUTION
};

/// the region position and length, and any alternative bases,
/// and the score too if it comes to that
class Mutation
{
public:
    Mutation(const Mutation&) = default;
    Mutation(Mutation&&) = default;

    Mutation& operator=(const Mutation&) = default;
    Mutation& operator=(Mutation&&) = default;

    // named constructors
    static Mutation Deletion(size_t start, size_t length);
    static Mutation Insertion(size_t start, char base);
    static Mutation Insertion(size_t start, const std::string& bases);
    static Mutation Insertion(size_t start, std::string&& bases);
    static Mutation Substitution(size_t start, char base);
    static Mutation Substitution(size_t start, const std::string& bases);
    static Mutation Substitution(size_t start, std::string&& bases);

    bool IsDeletion() const { return Type() == MutationType::DELETION; };
    bool IsInsertion() const { return Type() == MutationType::INSERTION; }
    bool IsSubstitution() const { return Type() == MutationType::SUBSTITUTION; }

    virtual bool IsScored() const { return false; }

    size_t Start() const { return start_; }
    size_t End() const { return start_ + length_; }
    size_t Length() const { return length_; }

    /// Returns the length difference introduced by this mutation.
    int LengthDiff() const
    {
        return boost::numeric_cast<int>(Bases().size()) - boost::numeric_cast<int>(length_);
    };

    size_t EditDistance() const { return std::max(Bases().size(), length_); }

    const std::string& Bases() const { return bases_; }
    MutationType Type() const { return type_; }

    /// Projects the Mutation from its original coordinates into the coordinate region
    /// defined by (start, length). If the mutation is outside this region, return none.
    boost::optional<Mutation> Translate(size_t start, size_t length) const;

    virtual bool operator==(const Mutation& other) const
    {
        if (Type() != other.Type() || Start() != other.Start()) return false;
        if (IsDeletion()) return Length() == other.Length();
        // insertion or substitution
        return Bases() == other.Bases();
    }

    virtual operator std::string() const;

    /// Uses this and the provided score to create and return a ScoredMutation.
    ScoredMutation WithScore(double score) const;

    /// Comparer to sort mutations by start/end.
    static bool SiteComparer(const Mutation& lhs, const Mutation& rhs)
    {
        // perform a lexicographic sort on End, Start, IsDeletion
        //   Deletions override everybody, so they get applied last,
        //   which implies, because false < true, use of !IsDeletion
        const auto l = std::make_tuple(lhs.End(), lhs.Start(), !lhs.IsDeletion());
        const auto r = std::make_tuple(rhs.End(), rhs.Start(), !rhs.IsDeletion());
        return l < r;
    }

private:
    std::string bases_;
    MutationType type_;
    size_t start_;
    size_t length_;

    Mutation(MutationType type, size_t start, size_t length);
    Mutation(MutationType type, size_t start, const std::string& bases);
    Mutation(MutationType type, size_t start, std::string&& bases);
    Mutation(MutationType type, size_t start, char);
};

class ScoredMutation : public Mutation
{
public:
    double Score;

    ScoredMutation(const ScoredMutation&) = default;
    ScoredMutation(ScoredMutation&&) = default;

    ScoredMutation& operator=(const ScoredMutation&) = default;
    ScoredMutation& operator=(ScoredMutation&&) = default;

    virtual bool IsScored() const override { return true; }
    virtual bool operator==(const ScoredMutation& other) const
    {
        return Score == other.Score && static_cast<const Mutation&>(*this) == other;
    }
    virtual bool operator==(const Mutation& other) const override { return false; }

    static bool ScoreComparer(const ScoredMutation& lhs, const ScoredMutation& rhs)
    {
        return lhs.Score < rhs.Score;
    }

private:
    ScoredMutation(const Mutation& mut, double score);

    // so Mutation can access the constructor
    friend class Mutation;
};

std::ostream& operator<<(std::ostream& out, MutationType type);
std::ostream& operator<<(std::ostream& out, const Mutation& mut);
std::ostream& operator<<(std::ostream& out, const ScoredMutation& smut);

std::string ApplyMutations(const std::string& tpl, std::vector<Mutation>* muts);
}
}  // ::PacBio::Consensus
