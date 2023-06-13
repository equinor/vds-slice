package grpc

import (
	"google.golang.org/grpc/resolver"
)

type resolverBuilder struct {
	scheme string
	serviceName string
	addresses []string
}

func (b *resolverBuilder) Build(
	target resolver.Target,
	cc resolver.ClientConn,
	opts resolver.BuildOptions,
) (resolver.Resolver, error) {
	r := &oneseismicResolver{
		target: target,
		cc:     cc,
		addrsStore: map[string][]string{
			b.serviceName: b.addresses,
		},
	}
	r.start()
	return r, nil
}

func (b *resolverBuilder) Scheme() string { return b.scheme }

func NewResolverBuilder(
	scheme string,
	serviceName string,
	addresses []string,
) *resolverBuilder {
	return &resolverBuilder{
		scheme: scheme,
		serviceName: serviceName,
		addresses: addresses,
	}
}

type oneseismicResolver struct {
	target     resolver.Target
	cc         resolver.ClientConn
	addrsStore map[string][]string
}

func (r *oneseismicResolver) start() {
	addrStrs := r.addrsStore[r.target.Endpoint()]
	addrs := make([]resolver.Address, len(addrStrs))
	for i, s := range addrStrs {
		addrs[i] = resolver.Address{Addr: s}
	}
	r.cc.UpdateState(resolver.State{Addresses: addrs})
}
func (*oneseismicResolver) ResolveNow(o resolver.ResolveNowOptions) {}
func (*oneseismicResolver) Close()                                  {}
