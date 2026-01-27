#ifndef STRING_UTILS_H
#define STRING_UTILS_H

#include <QString>
#include <algorithm>
#include <vector>

namespace Utils {

// Calculate Levenshtein distance between two strings
int levenshteinDistance(const QString& s1, const QString& s2);

// Calculate a simple fuzzy match score (0-100)
// Higher is better.
int calculateFuzzyScore(const QString& query, const QString& target);

}  // namespace Utils

#endif  // STRING_UTILS_H
