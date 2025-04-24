# Sketching

## Steps to run

1. Install python-matplotlib
2. Run `make -B` to compile.
3. `./test N PHI <cs|mg|cms>` (Default is MisraGries (mg))
3. Run `python3 generate-plot.py` to run all the various tests and save the data.

## Report

This project investigated three algorithms designed for data streams—Misra-Gries, Count-Min Sketch, and Count Sketch—to evaluate their effectiveness in identifying "heavy hitters" (high-frequency items) within large datasets. The experiments used data following a Zipfian distribution (s=1.5), simulating scenarios like word frequencies, with parameters N=100 million items and a universe size U=2³⁰. A key finding was that Misra-Gries and Count-Min Sketch significantly outperformed Count Sketch for this specific workload and achieved accuracy with a considerably lower memory footprint compared to naive hash maps.

### Methodology

#### Theoretical Parameter Determination: Determining K

Selecting an appropriate value for k (for finding the top-k items) was a key initial step. Leveraging the properties of the Zipfian distribution, I could estimate the frequency decay pattern (approximately 1/i^{s} where s=1.5). The goal was to find the rank k where the frequency just exceeds the threshold \phi N.

The frequency of the i-th element in a Zipfian distribution can be approximated as \frac{N}{Z} \cdot \frac{1}{i^{s}}, where Z is the sum of frequencies for the whole distribution, Z = \sum_{i=1}^{U} \frac{1}{i^{1.5}}.

We need the frequency of the k-th element to be greater than or equal to the threshold:

$$

\frac{N}{Z} \cdot \frac{1}{k^{1.5}} \geq \phi N

$$

Solving this inequality for k yields:

$$

k^{1.5} \leq \frac{1}{\phi Z}

$$

$$

k \leq \left(\frac{1}{\phi Z}\right)^{2/3}

$$

For large values of U (like the U=2³⁰ used here), Z can be effectively approximated using the Riemann zeta function, \zeta(s). For s=1.5, \zeta(1.5) \approx 2.6124.

```python
Python 3.13.2 (main, Feb  5 2025, 08:05:21) [GCC 14.2.1 20250128] on linux
Type "help", "copyright", "credits" or "license" for more information.
>>> from scipy.special import zeta
>>> zeta(1.5)
np.float64(2.612375348685488)
>>>
```

Plugging this value (approximated as 2.6123 in my implementation for convenience) into the equation allowed for an estimation of k. Based on testing, this estimate proved quite accurate for the large N and U values used. It's important to note this approximation might be less accurate for significantly smaller N or U values.

Accurate determination of k is crucial. If k is set too high, the results may include non-heavy hitters (false positives). If k is too low, true heavy hitters might be missed (false negatives). Using this estimation method, I observed very few false negatives in tests where the sketches performed well.

#### Misra-Gries

For implementing Misra-Gries, I followed the standard algorithm and used a C++ unordered_map to store counts for candidate heavy hitters, which provides efficient lookups. The parameter k was calculated using the formula derived above. Initially, the map size limit was set to just k, but this resulted in minimal memory usage compared to the other sketches. To ensure a fairer comparison in memory-based tests, the effective map size limit (k2 in the struct) was increased to k multiplied by a constant factor (50 in some tests). For the final output, only the top k elements based on count were returned from the map.

```c
typedef struct {
  std::unordered_map<u64, u64> *map;
  u64 k;
  u64 k2;
} MisraGries;
```

#### Count Sketch

For Count Sketch, I stored an array of hash function pairs (HashPair), one seed for determining the bucket index and another for the sign (+1 or -1). The sketch counters (slots) were implemented as a fixed-size 2D array. Using a fixed size determined at compile time (via guarded macros adjustable through make flags) showed better performance in my observations, likely due to compiler optimizations, compared to dynamic allocation. The rationale behind the specific size choices for this array is detailed further in the 'Parameter Tuning' section. This choice means memory usage might be higher than necessary for smaller inputs, but the speed improvement was deemed worthwhile. The counter type needed to be int64 (i64) to accommodate negative values resulting from the sign hash. A MinHeap (described below) was used alongside the sketch to track the top k elements dynamically.

_Flow_: When an item arrives, it's processed by each hash pair. For the j-th pair, it hashes to a bucket using `seed_main[j]` and gets a sign using `seed_sign[j]`. The corresponding counter `slots[j][bucket]` is updated by adding the sign. The frequency estimate for an item is the median of its signed counts across all d rows.

```c
typedef struct {
  u64 seed_main;
  u64 seed_sign;
} HashPair;

typedef struct {
  HashPair seeds[NUM_HASH_FUNCTION_PAIRS];
  u64 k;
  i64 slots[NUM_HASH_FUNCTION_PAIRS][CS_NUM_BUCKETS];
  MinHeap* heap;
} CountSketch;
```

#### Count-Min Sketch

The implementation of Count-Min Sketch closely mirrored that of Count Sketch but with key differences reflecting the algorithm's mechanics. It uses only one set of hash functions (no sign hashes), and the counters (slots) are uint64 since counts only increase. Similar to Count Sketch, compile-time fixed sizes were used for the counter array for performance reasons.

_Flow_: When an item arrives, it's hashed into a bucket for each of the d hash functions (using seeds `m[j]`). The counter in the corresponding `slots[j][bucket]` is incremented by 1. To estimate an item's frequency, the minimum count across all d corresponding buckets is taken.

```C
// Structure for Count-Min Sketch
typedef struct {
  u64 m[NUM_HASH_FUNCTIONS];
  u64 k;
  u64 slots[NUM_HASH_FUNCTIONS][NUM_BUCKETS];
  MinHeap *heap;
} CountMinSketch;
```

#### Top K Heavy Hitters Tracking (for CMS and CS)

For Misra-Gries, finding the top k is trivial: select the k entries with the highest counts from the final map. However, Count-Min Sketch and Count Sketch require an auxiliary structure to maintain the top k candidates seen so far, as the sketches themselves don't explicitly store item identifiers.

I used a combination of a MinHeap and a HashTable (`std::unordered_map`) for this purpose. The MinHeap stores at most k items, ordered by their estimated frequency (the heap's key). When a new item's frequency is estimated:

1. If the heap has fewer than k items, the new item is added.

2. If the heap is full and the new item's estimated frequency is greater than the frequency of the item at the top of the MinHeap (the current minimum among the top k), the minimum item is removed (popped) and the new item is added.

3. The HashTable is used to efficiently check if an item already exists in the heap. If an item seen again needs its count updated within the heap, the HashTable allows finding it quickly. The item can then be removed and reinserted with its updated count, maintaining the heap property, without needing a full heap traversal.


I initially experimented with third-party libraries like `uthash` and `sc_heap` but encountered some issues, leading me to implement this using standard C++ containers (`std::unordered_map` and a custom heap implementation), which proved reliable and reusable for both Count Sketch and Count-Min Sketch.

### Parameter Tuning

Number of Hash Functions (d)

I chose d=5 hash functions for both Count Sketch and Count-Min Sketch. This decision was based on the theoretical bound aiming for an error probability \delta \leq 0.01. The formula from lecture notes suggests:

$$

d = \lceil \log_e \frac{1}{\delta} \rceil

$$

$$

d = \lceil \log_e \frac{1}{0.01} \rceil = \lceil \log_e 100 \rceil = \lceil 4.605 \rceil = 5

$$

Number of Buckets (w)

Determining the optimal number of buckets w was more complex and algorithm-dependent:

- **Count-Min Sketch**: Theory suggests $w \approx 2/\epsilon$. For a target error $\epsilon = 0.001$ (0.1%), this gives w = 2000. For potential performance benefits (compiler optimizations for powers of 2), I used w=2048 in my implementation.

- **Count Sketch**: The optimal w for Count Sketch theoretically depends on the second frequency moment (F2) of the data distribution. For Zipfian distributions, F2 can be very large, suggesting a very large w (potentially in the order of 100k) might be needed. I tested configurations with large w, consuming significant memory, but did not observe substantial accuracy improvements for this specific task. Therefore, to maintain consistency for comparisons, I primarily used w=2048 for Count Sketch as well in the non-memory-focused tests.

- **Misra-Gries**: We know that the upper bound for size is $\frac{1}{\epsilon} - 1 \approx \frac{1}{\epsilon}$ . For $\epsilon = 0.001$ we have $w = 1000$, For the purpose of the assignment I used $w = k \cdot 50$ for keeping memory usage consistent across other sketch types.

## Experimental Results
An interesting observation was that precision and recall values were identical throughout most tests. This occurred because the algorithms were configured to output exactly k items (based on the estimated k). If an algorithm reported a false positive, it necessarily meant a true positive was replaced by the false positive, resulting in an equal number of false positives and false negatives. Therefore, Precision = TP / (TP + FP) became equal to Recall = TP / (TP + FN). For determining correctness, an item's count estimate was allowed a 1% (\delta = 0.01) margin compared to its ground truth count to be considered a true positive.

##### NOTE: All results were run on my local machine:
I had issues trying to run it on khoury servers, I hope there is some consideration. There maybe discrepancies in timing but it should be the same relative to each other.
```
CPU: AMD Ryzen 7 5800x3d 4.7GHZ
RAM: 48GB
OS: Arch Linux 6.14.2-arch1-1
gcc version 14.2.1 20250207 (GCC)
Caches (sum of all):
  L1d:                    256 KiB (8 instances)
  L1i:                    256 KiB (8 instances)
  L2:                     4 MiB (8 instances)
  L3:                     96 MiB (1 instance)
```

### Precision & Recall vs PHI
The x-axis has values for phi from 0.001 to 0.01 and y-axis has precision and recall.

Parameters:
N = 100M
Num Buckets = 2048
Num Hash Functions = 5
Size for MisraGries = 2600

![[https://github.com/Rohansjamadagn/Sketching/blob/master/phi_sensitivity.png]]

We see that CMS (Count Min Sketch) performs the best compared to the other two. CS (Count Sketch) results have large variance because of the nature of the algorithm and distribution. MIsra Gries although behind CMS, it performs consistently and as we'll see further, when given more memory it is on par with CMS.

### Precision and Recall vs Memory (Space) usage

The y-axis show the precision/recall percentage and the x-axis shows the sketch size in bytes for each approach. The parameters used here was:

N = 100M
$\phi$ = 0.001
For Count Sketch and Count Min Sketch the number of hash functions was fixed at 5
- Number of bucketes tested = \{512, 1024, 2048, 4096, 8192\}
For Misra Gries, the number of buckets was k * {50, 100, 200, 400}, for this test k = 52, and therefore the total buckets were = {2600, 5200, 10400, 20800}


![[https://github.com/Rohansjamadagn/Sketching/blob/master/memory_ananlysis.png]]

After seeing these results I realized that I had tested Misra Gries with worse conditions so I went back and re-did the Phi tests with number of buckets = 5200 which was closer to the memory usage of Count Min Sketch and Count Sketch.

![[https://github.com/Rohansjamadagn/Sketching/blob/master/phi_sensitivity_2.png]]

And we can see that Misra Gries Performs in line with Count Min Sketch.

Comparison to Map: Nonetheless these approaches take several orders of magnitude less memory than using the HashMap. I ran the test without setting up the sketch and recorded the peak memory usage to be around 4.8GB, which is significantly higher than the 100s of kilobytes used here.

### Streaming Time vs Sketch Size

Y-axis shows Sketch size in bytes and X-axis shows streaming time in seconds

Parameters:
N = 100M
Num Buckets = 2048
Num Hash Functions = 5
Size for MisraGries = 5200


![[https://github.com/Rohansjamadagn/Sketching/blob/master/time_analysis.png]]


We can clearly see that Count Sketch takes significantly more time averaging around 4s. Count Min sketch does better at around 1.8s. Misra Gries performs the best with a latency under a second. The red dotted line shows that the count time using std::map is much better than any of the sketching approaches. This is presumably due to optimizations and smaller number of compuatation needed to be done i.e only hashing once vs multiple times mentioned here. The hash functioned used in map is also presumably much faster than Murmur hash that is used by my implementation.

### Precision and Recall vs Sketch Time

The y-axis shows precision and recall (both have same values due to reasons mentioned earlier), and the x axis shows update times.

Parameters:
N = 100M
Num Buckets = 2048
Num Hash Functions = 5
Size for MisraGries = 5200

![[https://github.com/Rohansjamadagn/Sketching/blob/master/recall_vs_update_time.png]]
![[https://github.com/Rohansjamadagn/Sketching/blob/master/precision_vs_update_time.png]]

These results re-iterate the information mentioned in the previous sections. The thing to note is that there are some random outliers in this case.

### Discussion of Trade-offs

#### Accuracy vs. Performance (Theoretical Guarantees vs. Empirical Performance)

Misra-Gries provides a deterministic guarantee: it identifies all items x with true frequency $f(x) > \phi N$. This guarantee requires O(k) space, where k = $\lfloor 1/\phi \rfloor$. The memory usage grows linearly as we aim to find less frequent items (smaller $\phi$). My empirical results strongly validated this guarantee, showing perfect recall when the algorithm was appropriately sized (e.g., using the $k \times 100$ factor which provided sufficient space). While the algorithm underestimates counts $(f(x) - \phi N \leq \hat{f}_{MG}(x) \leq f(x))$, this did not prevent identification of items clearly above the $\phi N$ threshold in these tests. Its fast empirical performance (<1s stream time) also aligns with its typically efficient update mechanism (often O(1) amortized).

Count-Min Sketch offers probabilistic guarantees. For any item x, the estimated count $\hat{f}_{CMS}(x)$ satisfies $f(x) \leq \hat{f}_{CMS}(x) \leq f(x) + \epsilon N$ with probability at least $1-\delta$. The parameters $\epsilon$ and $\delta$ are determined by the sketch dimensions: $\epsilon \approx 2/w$ (or $e/w$) and $\delta \approx (1/e)^d$. The space complexity is O($w \times d$) = O($\frac{1}{\epsilon} \log \frac{1}{\delta}$). My empirical results align well: accuracy improved substantially as w increased (from 512 to 4096), consistent with a decreasing error bound $\epsilon$. The 94-100% accuracy achieved with w=2048 and d=5 corresponds to theoretical parameters $\epsilon \approx 2/2048 \approx 0.001$ and $\delta \approx (1/e)^5 \approx 0.0067$, matching the target error levels. The systematic overestimation bias inherent in the guarantee ($\hat{f}_{CMS}(x) \geq f(x)$) did not significantly impair heavy hitter identification for the skewed Zipfian data, where heavy hitters have substantially higher frequencies than other items.

Count Sketch provides an unbiased estimate, $E[\hat{f}_{CS}(x)] = f(x$), and bounds the variance: $Var[\hat{f}_{CS}(x)] \leq F_2 / w$, where $F_2 = \sum_i f(i)^2$ is the second frequency moment. Its space complexity is O($w \times d$) = O($\frac{1}{\epsilon^2} \log \frac{1}{\delta}$), where the error guarantee is often related to estimating $F_2$ or achieving $\hat{f}_{CS}(x) \approx f(x) \pm \epsilon \sqrt{F_2}$. While theoretically sound, the empirical performance was poor (26-48% precision/recall). This suggests that for the Zipfian distribution (s=1.5), $F_2$ is large, making the variance $F_2/w$ substantial even with w=2048. Consequently, individual estimates deviated significantly from the true counts, rendering the unbiased property less useful in practice for reliably identifying the top-k items compared to the biased but bounded estimates of CMS or the deterministic guarantees of MG for this specific task and dataset. Its slower empirical performance (~4s stream time) also detracted from its practicality here.

#### Estimation Biases

Misra-Gries tends to _underestimate_ frequencies due to its counter decrement mechanism. This conservative bias makes it less likely to report non-heavy hitters but introduces a risk of missing true heavy hitters (false negatives) if not sized correctly. My results indicated this risk could be managed through proper parameter selection (k).

Count-Min Sketch inherently _overestimates_ counts because it takes the minimum value from counters that may have been incremented by colliding items. However, for the Zipfian distribution used, this bias primarily affected lower-frequency items, leaving the top-k detection largely unaffected, as reflected in the high precision scores.

Count Sketch provides theoretically _unbiased_ median estimates. However, this average property doesn't preclude high variance in individual estimates. In practice, my results showed high variance, with estimates often falling significantly above or below the true count. This likely explains its poor performance on the skewed dataset, where collisions and hash conflicts might have had a more pronounced effect than the theory averages out.

#### Interesting Findings
An unexpected aspect of this experiment was the strong performance of Misra-Gries. Given its relatively straightforward implementation compared to the sketch-based methods, I initially anticipated it might perform less effectively. However, for this specific Zipfian distribution, it proved quite capable. Similarly, while Count-Min Sketch is known to overestimate counts, its accuracy in these tests was high, showing minimal practical error despite the theoretical bias. While the theoretical guarantees discussed earlier generally held, applying these algorithms to the distribution revealed interesting nuances in their practical behavior.
### Conclusion

The key findings from this analysis provide insights for selecting algorithms for heavy hitter detection:

1. Misra-Gries is highly effective and provides guarantees, making it an excellent choice when memory resources are sufficient for the required k.
2. Count-Min Sketch offered the best practical balance of accuracy and efficiency for the skewed Zipfian distribution tested.
3. Count Sketch performed poorly for this specific task, potentially because its theoretical advantages (unbiasedness, L2 norm guarantees) are less critical than controlling variance for heavy hitter identification in highly skewed distributions like Zipfian(1.5). It might be better suited for applications directly leveraging its specific properties like adversarial loads and for distributions with lower F2 scores.
