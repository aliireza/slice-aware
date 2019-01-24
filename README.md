# Slice-aware Memory Management

Applications can utilize our memory management scheme to improve their performance by allocating memory that is mapped to the most appropriate LLC slice(s), i.e., that have lower access latency. For more information, check out our [paper][cachedirector-eurosys-paper].

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
 author = {Farshin, Alireza and Roozbeh, Amir and Maguire Jr., Gerald Q. and Kosti\'{c}, Dejan},
 title = {Make the Most out of Last Level Cache in Intel Processors},
 booktitle = {Proceedings of the Fourteenth EuroSys Conference},
 series = {EuroSys '19},
 year = {2019},
 isbn = {978-1-4503-6281-8/19/03},
 location = {Dresden, Germany},
 doi = {10.1145/3302424.3303977},
 acmid = {3303977},
 publisher = {ACM},
 address = {New York, NY, USA},
}
```

## Getting Help

If you have any questions regarding our code or paper, you can contact Amir Roozbeh (amirrsk at kth.se) and/or Alireza Farshin (farshin at kth.se).

[cachedirector-eurosys-paper]: http://doi.org/10.1145/3302424.3303977
[cachedirector-readme]: https://github.com/aliireza/dpdk/blob/cachedirector/README
[cachedirector-repo]: https://github.com/aliireza/dpdk
