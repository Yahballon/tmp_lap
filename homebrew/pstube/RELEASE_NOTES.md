# PSTUBE v0.5.2 — Search continuation beyond page 2

- Fix the 38-video cutoff: YouTube's later search pages can encode their pagination token as a direct continuationCommand or nextContinuationData rather than continuationItemRenderer.
- Recognize multiple real continuation response shapes and keep progressively fetching more pages while valid next tokens exist.
- Prevent looping on a repeated continuation token.
- Extend the native C++ live search regression to verify at least 60 real videos over 3+ pages.
- Preserve all native player, thumbnail, favourites and downloads functionality.

No PC or external helper required. YouTube's public API can still change or throttle requests; results depend on the responses actually available.
