#ifndef CGAL_KD_KERNEL_3_INTERFACE_H
#define CGAL_KD_KERNEL_3_INTERFACE_H

#include <CGAL/functor_tags.h>
#include <CGAL/transforming_iterator.h>
#include <CGAL/marcutils.h>
#include <CGAL/tuple.h>


namespace CGAL {
template <class Base_> struct Kernel_3_interface : public Base_ {
	typedef Base_ Base;
	typedef Kernel_3_interface<Base> Kernel;
	typedef typename Base::Point   Point_3;
	typedef typename Base::Vector  Vector_3;
	typedef typename Base::Segment Segment_3;
	typedef cpp0x::tuple<Point_3,Point_3,Point_3> Triangle_3; // placeholder
	typedef cpp0x::tuple<Point_3,Point_3,Point_3,Point_3> Tetrahedron_3; // placeholder
	struct Compare_xyz_3 {
		typedef typename Base::template Functor<Compare_lexicographically_tag>::type CL;
		typedef typename CL::result_type result_type;
		CL cl;
		Compare_xyz_3(Kernel const&k):cl(k){}
		result_type operator()(Point_3 const&a, Point_3 const&b) {
			return cl(a,b);
		}
	};
	struct Compare_distance_3 {
		typedef typename Base::template Functor<Compare_distance_tag>::type CD;
		typedef typename CD::result_type result_type;
		CD cd;
		Compare_distance_3(Kernel const&k):cd(k){}
		result_type operator()(Point_3 const&a, Point_3 const&b, Point_3 const&c) {
			return cd(a,b,c);
		}
		result_type operator()(Point_3 const&a, Point_3 const&b, Point_3 const&c, Point_3 const&d) {
			return cd(a,b,c,d);
		}
	};
	struct Orientation_3 {
		typedef typename Base::template Functor<Orientation_of_points_tag>::type O;
		typedef typename O::result_type result_type;
		O o;
		Orientation_3(Kernel const&k):o(k){}
		result_type operator()(Point_3 const&a, Point_3 const&b, Point_3 const&c, Point_3 const&d) {
			//return o(a,b,c,d);
			Point_3 const* t[4]={&a,&b,&c,&d};
			return o(make_transforming_iterator<Dereference_functor>(t+0),make_transforming_iterator<Dereference_functor>(t+4));

		}
	};
	struct Side_of_oriented_sphere_3 {
		typedef typename Base::template Functor<Side_of_oriented_sphere_tag>::type SOS;
		typedef typename SOS::result_type result_type;
		SOS sos;
		Side_of_oriented_sphere_3(Kernel const&k):sos(k){}
		result_type operator()(Point_3 const&a, Point_3 const&b, Point_3 const&c, Point_3 const&d, Point_3 const&e) {
			//return sos(a,b,c,d);
			Point_3 const* t[5]={&a,&b,&c,&d,&e};
			return sos(make_transforming_iterator<Dereference_functor>(t+0),make_transforming_iterator<Dereference_functor>(t+5));
		}
	};

	// I don't have the Coplanar predicates (yet)


	Compare_xyz_3 compare_xyz_3_object()const{ return Compare_xyz_3(*this); }
	Compare_distance_3 compare_distance_3_object()const{ return Compare_distance_3(*this); }
	Orientation_3 orientation_3_object()const{ return Orientation_3(*this); }
	Side_of_oriented_sphere_3 side_of_oriented_sphere_3_object()const{ return Side_of_oriented_sphere_3(*this); }
};
}

#endif
