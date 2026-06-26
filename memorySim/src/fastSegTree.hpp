#pragma once

template<size_t N>
class FastSegTree {
	static const int len = 2*N;
	uint64_t v[len];
	uint64_t minInd[len];
	void build(){
		for(int i=0;i<N;i++){
			minInd[i] = i;
		}
		for(int i=N-1;i>=0;i--){
			if(v[i*2] < v[i*2+1]) {
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
	void update(int i, int val){
		i+=N;
		v[i] = val;
		i/=2;
		while(i>0){
			if(v[i*2] < v[i*2+1]) {
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
