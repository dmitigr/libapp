-- -*- SQL -*-
--
-- Copyright 2025 Dmitry Igrishin
--
-- Licensed under the Apache License, Version 2.0 (the "License");
-- you may not use this file except in compliance with the License.
-- You may obtain a copy of the License at
--
--     http://www.apache.org/licenses/LICENSE-2.0
--
-- Unless required by applicable law or agreed to in writing, software
-- distributed under the License is distributed on an "AS IS" BASIS,
-- WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
-- See the License for the specific language governing permissions and
-- limitations under the License.

-- This (unrelated) comment is a part of the first query that follows it.
-- But the metadata specified in this comment is not a part of the metadata
-- specified in the related comment below.
-- $id$nonsence-since-the-comment-is-unrelated$id$

-- This query calculates :{n} + 1.
--
-- $id$plus_one$id$
SELECT :{n}::int + 1, 'semicolons in qoutes like these: ;;; are ignored';

/*
 * This query generates a numeric sequence.
 *
 * $id$digit$id$
 *
 * $cond$
 * n > 0
 *   AND n < 2
 * $cond$
 */
SELECT n FROM (SELECT generate_series(1,9) n) foo WHERE :{cond};

-- This is an empty statement.
--
-- $id$empty-statement$id$
;

-- This is a placeholder for any data.
--
-- $id$$i$any-data$$$id$
:{data}
