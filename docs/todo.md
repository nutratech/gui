Based on your current setup—which utilizes `usda-sqlite` to process the USDA SR-Legacy database—you have a solid foundation for commodity ingredients (raw fruits, vegetables, meats). However, SR-Legacy is static (stopped updating in 2018) and lacks the vast universe of branded products (UPC scanning) and modern micronutrient density data.

To augment this for a "good user experience" (effortless search + rich data), you should integrate the following open-source and public APIs.

### 1. USDA FoodData Central (FDC) API

Since your project is already built on USDA logic, this is the most seamless integration. SR-Legacy (which you currently use) is now just one subset of FoodData Central.

* **Why use it:** It includes **"Branded Foods"** (over 300,000 items from labels) and **"Foundation Foods"** (newer, highly granular micronutrient data that replaces SR-Legacy).
* **Integration Strategy:**
* Your `food_des` table uses `fdgrp_id`. FDC uses `fdcId`. You can create a mapping table to link your legacy IDs to new FDC IDs.
* **Search:** The FDC API allows for keyword search which returns `fdcId`. You can use this to backfill missing nutrients in your `nut_data` table.

* **Cost/License:** Free, Public Domain (U.S. Government).

### 2. Open Food Facts (OFF)

This is the "Wikipedia of food." It is the best open-source resource for scanning barcodes and finding branded products internationally.

* **Why use it:**
* **Barcodes:** It relies heavily on UPC/EAN codes. If your UX involves a camera/scanner, this is essential.
* **Crowdsourced:** It covers niche brands that the USDA might miss.
* **Nutri-Score:** It provides calculated scores (Nutri-Score, NOVA group) which you can store in your `food_des` or a new `food_metadata` table.

* **Integration Strategy:**
* Query by barcode: `https://world.openfoodfacts.org/api/v0/product/[barcode].json`
* Map their JSON `nutriments` object to your `nutr_def` IDs (e.g., map OFF `saturated-fat_100g` to your `nutr_def` ID for saturated fat).

* **Cost/License:** Free, Open Database License (ODbL).

### 3. SQLite FTS5 (Full-Text Search)

You are currently using SQLite. To make search "effortless" without calling an external API for every keystroke, you should utilize SQLite's native Full-Text Search extension.

* **Why use it:** Your `food_des` table contains `long_desc`, `shrt_desc`, and `com_name`. Standard SQL `LIKE` queries are slow and bad at matching "natural" language. FTS5 allows for lightning-fast, ranked search results (e.g., typing "chk brst" finds "Chicken Breast").
* **Integration:**
* Modify your `sql/tables.sql` to include a virtual table:

```sql
CREATE VIRTUAL TABLE food_search USING fts5(long_desc, shrt_desc, com_name, content=food_des, content_rowid=id);

```

* Add a trigger to keep it updated. This will make your local search instant.

### 4. Natural Language Processing (NLP) Parsers

If "effortless search" means the user types "1 cup of oatmeal with 2 tbsp honey," you need a parser, not just a database.

* **New York Times Ingredient Phrase Tagger (CRF++):** A structured learning model released by NYT to parse ingredient lines into amount, unit, and food.
* **Price:** Open Source (Apache 2.0).
* **Integration:** You can run this as a microservice (Python) alongside your `process.py`. When a user types a sentence, parse it to extract the quantity (to calculate `gram` weight) and the food subject (to query your SQLite DB).

### Recommended Architecture Update

Given your file structure, here is how you should integrate these sources:

1. **Augment `nutr_def`:** Ensure your `nutr_def` table aligns with Open Food Facts tag naming conventions (add a column `off_tag` to map `fat` -> `fat_100g`).
2. **New Table `external_links`:**
Don't pollute `food_des` with mixed data. Create a linking table:

```sql
CREATE TABLE external_links (
  local_food_id INT,
  service_name TEXT, -- 'FDC', 'OFF'
  external_id TEXT,  -- '123456' or UPC
  last_updated DATETIME,
  FOREIGN KEY(local_food_id) REFERENCES food_des(id)
);

```

1. **Python Script (`data/process.py` extension):**
Write a new script (e.g., `fetch_external.py`) that:

* Takes a user query.
* Checks local SQLite FTS5 first.
* If no hit, queries Open Food Facts API.
* Inserts the result into `food_des` and `nut_data`, and saves the link in `external_links`.
