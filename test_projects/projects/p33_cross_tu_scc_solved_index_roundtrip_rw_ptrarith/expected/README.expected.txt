C4G expected behavior: solved recursive-SCC summaries are materialized in the
project summary index and preferred during later import-mode runs.

This is the same project shape as p22:
  f -> relay -> g -> f

Required workflow:
  1. run summaries for both TUs;
  2. build project-summary-index.json;
  3. rerun in import mode to let C4E refine f/g inside each TU;
  4. rebuild project-summary-index.json from those improved summaries;
  5. rerun import mode again.

Expected index-level evidence after step 4:
  project-summary-index.json should contain solvedParamSummaries entries for
  recursive SCC members such as f, relay, and g when their parameter summaries
  are non-empty.

Expected visitor behavior after step 5:
  imports of g from a.c should prefer g's solvedParamSummaries from the index
  rather than the stale raw summary snapshot, allowing relay to see more than
  only the first int* step when the index was rebuilt from improved summaries.

This test is multi-TU, so it intentionally remains under test_projects/projects.
