#include "utils/string_utils.h"
#include <QRegularExpression>
#include <QStringList>
#include <QtGlobal>
#include <algorithm> // Required for std::max
#include <cmath>

#ifndef QT_VERSION_CHECK
#define QT_VERSION_CHECK(major, minor, patch)                                  \
  ((major << 16) | (minor << 8) | (patch))
#endif

namespace Utils {

int levenshteinDistance(const QString &s1, const QString &s2) {
  const auto m = s1.length();
  const auto n = s2.length();

  std::vector<std::vector<int>> dp(m + 1, std::vector<int>(n + 1));

  for (int i = 0; i <= m; ++i) {
    dp[i][0] = i;
  }
  for (int j = 0; j <= n; ++j) {
    dp[0][j] = j;
  }

  for (int i = 1; i <= m; ++i) {
    for (int j = 1; j <= n; ++j) {
      if (s1[i - 1] == s2[j - 1]) {
        dp[i][j] = dp[i - 1][j - 1];
      } else {
        dp[i][j] = 1 + std::min({dp[i - 1][j], dp[i][j - 1], dp[i - 1][j - 1]});
      }
    }
  }

  return dp[m][n];
}

int calculateFuzzyScore(const QString &query, const QString &target) {
  if (query.isEmpty()) {
    return 0;
  }
  if (target.isEmpty()) {
    return 0;
  }

  QString q = query.toLower();
  QString t = target.toLower();

  // 1. Exact match bonus
  if (t == q) {
    return 100;
  }

  // 2. Contains match bonus (very strong signal)
  if (t.contains(q)) {
    return 90; // Base score for containing the string
  }

  // 3. Token-based matching (handling "grass fed" vs "beef, grass-fed")
  static const QRegularExpression regex("[\\s,-]+");
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
  auto behavior = Qt::SkipEmptyParts;
#else
  auto behavior = QString::SkipEmptyParts;
#endif
  QStringList queryTokens = q.split(regex, behavior);
  QStringList targetTokens = t.split(regex, behavior);

  int totalScore = 0;
  int matchedTokens = 0;

  for (const QString &qToken : queryTokens) {
    int maxTokenScore = 0;
    for (const QString &tToken : targetTokens) {
      int dist = levenshteinDistance(qToken, tToken);
      int maxLen = static_cast<int>(std::max(qToken.length(), tToken.length()));
      if (maxLen == 0)
        continue;

      int score = 0;
      if (tToken.startsWith(qToken)) {
        score = 95; // Prefix match is very good
      } else {
        double ratio = 1.0 - (static_cast<double>(dist) / maxLen);
        score = static_cast<int>(ratio * 100);
      }

      maxTokenScore = std::max(maxTokenScore, score);
    }
    totalScore += maxTokenScore;
    if (maxTokenScore > 60)
      matchedTokens++;
  }

  if (queryTokens.isEmpty()) {
    return 0;
  }

  int averageScore = static_cast<int>(totalScore / queryTokens.size());

  // Penalize if not all tokens matched somewhat well
  if (matchedTokens < queryTokens.size()) {
    averageScore -= 20;
  }

  return std::max(0, averageScore);
}

} // namespace Utils
