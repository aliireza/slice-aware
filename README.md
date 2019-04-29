# Slice-aware Memory Management

Applications can utilize our memory management scheme to improve their performance by allocating memory that is mapped to the most appropriate LLC slice(s), i.e., that have lower access latency. For more information, check out our [paper][cachedirector-eurosys-paper] and my talk at [EuroSys'19][cachedirector-video].

## Download

```
git clone --recursive https://github.com/aliireza/slice-aware.git
```

## Repository Organization

The description of different folders are as follows: 

- `./apps/` Contains applications for measuring access time to different LLC slices + Finding the mapping between different physical addresses & LLC slices.
-  `./cachedirector/` Implementation of CacheDirector on DPDK. More info can be found in [here][cachedirector-readme].
- `./lib/` Contains libraries used to enable slice-aware memory management.
- `./others/` Some useful information regarding the CAT and the mapping of core<->slice(s) for Intel(R) Xeon(R) Gold 6134 (Skylake).
- `./results/` Contains some of the results used in our [paper][cachedirector-eurosys-paper].
- `./workload/` Contains some sample access patterns (i.e., Uniform random and Zipf) and the source code for generating them.

## Build & Run

- To build applications and workload-generators, you can use the `Makefile` available in `./apps/` and `./workload/generator/`.
- For running each application, please make sure that you are passing the right arguments. More information can be found in source code.
- For CacheDirector, please refer to [here][cachedirector-readme].


## Citing our paper

If you use [CacheDirector][cachedirector-repo] or slice-aware memory management in any context, please cite our [paper][cachedirector-eurosys-paper]:

```
@inproceedings{farshin-slice-aware,
 author = {Farshin, Alireza and Roozbeh, Amir and {Maguire Jr.}, Gerald Q. and Kosti\'{c}, Dejan},
 title = {{Make the Most out of Last Level Cache in Intel Processors}},
 booktitle = {Proceedings of the Fourteenth EuroSys Conference 2019},
 series = {EuroSys '19},
 year = {2019},
 isbn = {978-1-4503-6281-8},
 location = {Dresden, Germany},
 pages = {8:1--8:17},
 articleno = {8},
 numpages = {17},
 url = {http://doi.acm.org/10.1145/3302424.3303977},
 doi = {10.1145/3302424.3303977},
 acmid = {3303977},
 publisher = {ACM},
 address = {New York, NY, USA},
 keywords = {Cache Allocation Technology, Cache Partitioning, CacheDirector, DDIO, DPDK, Key-Value Store, Last Level Cache, Network Function Virtualization, Non-Uniform Cache Architecture, Slice-aware Memory Management},
}
```

## Getting Help

If you have any questions regarding our code or the paper, you can contact Amir Roozbeh (amirrsk at kth.se) and/or Alireza Farshin (farshin at kth.se).

[cachedirector-eurosys-paper]: https://people.kth.se/~farshin/documents/slice-aware-eurosys19.pdf
[cachedirector-video]: https://play.kth.se/media/Make+the+Most+out+of+Last+Level+Cache+in+Intel+Processors+%28EuroSys+%2719%29/0_jqd1pfa9
[cachedirector-readme]: https://github.com/aliireza/CacheDirector/blob/cachedirector/README.md
[cachedirector-repo]: https://github.com/aliireza/CacheDirector
