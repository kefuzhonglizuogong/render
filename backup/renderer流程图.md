
flowchart TD

&#x20;   A\["trace(ray)"] --> B\["for each bounce"]

&#x20;   B --> C{"scene.intersect(ray) ?"}

&#x20;   C -->|No| D{"hit environment/background"}

&#x20;   C -->|Yes| E{"rec.material exists ?"}

&#x20;   E -->|No| Z\["break"]

&#x20;   E -->|Yes| F{"emitted != black ?"}



&#x20;   D --> D1{"bounce == 0 ?"}

&#x20;   D1 -->|Yes| D2\["L += beta \* env"]

&#x20;   D1 -->|No| D3{"previousWasBsdfSample ?"}

&#x20;   D3 -->|Yes| D4\["mis = misWeightForBsdfHitLight(...)"]

&#x20;   D4 --> D5\["L += beta \* env \* mis"]

&#x20;   D3 -->|No| D6\["L += beta \* env"]

&#x20;   D2 --> END1\["break"]

&#x20;   D5 --> END1

&#x20;   D6 --> END1



&#x20;   F -->|Yes| F1{"bounce == 0 ?"}

&#x20;   F1 -->|Yes| F2\["L += beta \* emitted"]

&#x20;   F1 -->|No| F3{"previousWasBsdfSample ?"}

&#x20;   F3 -->|Yes| F4\["mis = misWeightForBsdfHitLight(...)"]

&#x20;   F4 --> F5\["L += beta \* emitted \* mis"]

&#x20;   F3 -->|No| F6\["no add"]

&#x20;   F2 --> END2\["break"]

&#x20;   F5 --> END2

&#x20;   F6 --> END2



&#x20;   F -->|No| G\["directLight = estimateDirectLightMIS(...)"]

&#x20;   G --> H\["L += beta \* directLight"]

&#x20;   H --> I\["bsdfSample = material.sample(...)"]

&#x20;   I --> J{"bsdfSample valid and pdf > 0 ?"}

&#x20;   J -->|No| Z

&#x20;   J -->|Yes| K{"isDelta ?"}

&#x20;   K -->|Yes| K1\["beta \*= f"]

&#x20;   K -->|No| K2\["beta \*= f \* cos / pdf"]

&#x20;   K1 --> L\["store previousWasBsdfSample, previousWasDelta, previousBsdfPdf"]

&#x20;   K2 --> L

&#x20;   L --> M\["ray = new bounced ray"]

&#x20;   M --> B



