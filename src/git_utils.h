#ifndef __GIT_HEATMAP_GIT_UTILS_H__
#define __GIT_HEATMAP_GIT_UTILS_H__

template <typename T, typename Del>
class GitResourceGuard {
   public:
    GitResourceGuard(T* ref, Del del) : ref(ref), del(del) {}
    ~GitResourceGuard() {
        if (ref) {
            del(ref);
        }
    }
    T** operator&() { return &ref; }
    T* get() const { return ref; }

   private:
    T* ref;
    Del del;
};

#endif  // __GIT_HEATMAP_GIT_UTILS_H__
