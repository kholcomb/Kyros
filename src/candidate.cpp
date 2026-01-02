#include <kyros/candidate.hpp>
#include <cmath>

namespace kyros {

void Candidate::recalculate_confidence() {
    if (evidence.empty()) {
        confidence_score = 0.0;
        return;
    }

    // Count evidence by strength classification
    int definitive_count = 0;
    int strong_count = 0;
    int moderate_count = 0;
    int weak_count = 0;

    // RULE 1: Any negative evidence → confidence = 0
    // Negative evidence represents confirmed NOT MCP (e.g., Chromium IPC, LSP)
    for (const auto& e : evidence) {
        if (e.is_negative) {
            confidence_score = 0.0;
            return;
        }

        // Count evidence by strength
        switch (e.strength) {
            case Evidence::Strength::Definitive:
                definitive_count++;
                break;
            case Evidence::Strength::Strong:
                strong_count++;
                break;
            case Evidence::Strength::Moderate:
                moderate_count++;
                break;
            case Evidence::Strength::Weak:
                weak_count++;
                break;
        }
    }

    // RULE 2: Weak evidence alone capped at 0.49 (below active testing threshold)
    // This prevents single weak evidence (e.g., parent process only) from triggering
    // expensive active testing that leads to false positives
    if (weak_count > 0 && moderate_count == 0 && strong_count == 0 && definitive_count == 0) {
        // Calculate using Noisy-OR but cap result
        double product_of_negatives = 1.0;
        for (const auto& e : evidence) {
            product_of_negatives *= (1.0 - e.confidence);
        }
        confidence_score = std::min(0.49, 1.0 - product_of_negatives);
        return;
    }

    // RULE 3: Otherwise use normal Noisy-OR algorithm
    // Noisy-OR algorithm: Compound independent probabilities
    // Formula: P(true) = 1 - ∏(1 - P(evidence_i))
    //
    // This properly handles:
    // - Multiple independent signals compound to higher confidence
    // - Duplicate/similar evidence has diminishing returns
    // - Multiple strong signals can reach high confidence
    //
    // Examples:
    // - 1 signal at 70% → 70%
    // - 2 signals at 70% → 91% (1 - 0.3 * 0.3)
    // - 2 signals at 30% → 51% (1 - 0.7 * 0.7)
    // - 3 signals at 30% → 66% (1 - 0.7^3)

    double product_of_negatives = 1.0;
    for (const auto& e : evidence) {
        // Multiply (1 - confidence) for each piece of evidence
        product_of_negatives *= (1.0 - e.confidence);
    }

    // Final confidence is 1 minus the product
    confidence_score = 1.0 - product_of_negatives;

    // Cap at 0.99 to indicate "very high confidence" but never absolute certainty
    if (confidence_score > 0.99) {
        confidence_score = 0.99;
    }
}

} // namespace kyros
