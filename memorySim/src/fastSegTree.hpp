#pragma once
#include <cstdint>
#include <cstddef>

template<size_t N>
class FastSegTree {
private:
	static const int len = 2*N;
	uint64_t v[len];
	uint64_t minInd[len];
public:
	void build(){
		for(size_t i=0;i<N;i++){
			v[N+i] = 0;
			minInd[N+i] = i;
		}
		for(size_t i=N-1;i>=1;i--){
			if(v[i*2] <= v[i*2+1]) {
				v[i] = v[i*2];
				minInd[i] = minInd[i*2];
			}
			else {
				v[i] = v[i*2+1];
				minInd[i] = minInd[i*2+1];
			}
		}
	}
	int getMinIndex(){
		return minInd[1];
	}
	void update(size_t i, uint64_t val){
		i+=N;
		v[i] = val;
		i/=2;
		while(i>0){
			if(v[i*2] <= v[i*2+1]) {
				v[i] = v[i*2];
				minInd[i] = minInd[i*2];
			}
			else {
				v[i] = v[i*2+1];
				minInd[i] = minInd[i*2+1];
			}
			i/=2;
		}
	}
};
