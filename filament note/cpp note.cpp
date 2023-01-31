
// Any allocation function for a class T is a static member (even if not explicitly declared static)
// An operator new function defined for a class is a static member function (which can't be virtual) that hides the global operator new function for objects of that class type. 
class FilamentAPI 
{
    static void *operator new     (size_t) = delete;
    static void *operator new[]   (size_t) = delete;
    static void  operator delete  (void*)  = delete;
    static void  operator delete[](void*)  = delete;
};

class xyz
{
    void* operator new (size_t size); //implicitly declared static
    void operator delete (void *p); //implicitly declared static
};


//----------------------------------------------------------------------------------------------------------


class UTILS_PUBLIC IndexBuffer : public FilamentAPI 
{
    enum class IndexType : uint8_t 
    {
        // 把 ElementType 中定义的 enum 的值引用过来. 
        USHORT = uint8_t(backend::ElementType::USHORT),  
        UINT = uint8_t(backend::ElementType::UINT),      
    };

}

namespace filament::backend 
{
enum class ElementType : uint8_t 
{
    BYTE,
    BYTE2,
    BYTE3,
    BYTE4,
    UBYTE,
    UBYTE2,
    UBYTE3,
    UBYTE4,
    SHORT,
    SHORT2,
    SHORT3,
    SHORT4,
    USHORT,
    USHORT2,
    USHORT3,
    USHORT4,
    INT,
    UINT,
    FLOAT,
    FLOAT2,
    FLOAT3,
    FLOAT4,
    HALF,
    HALF2,
    HALF3,
    HALF4,
};
}

//----------------------------------------------------------------------------------------------------------


class UTILS_PUBLIC MaterialInstance : public FilamentAPI 
{
    // 此处用法可以学习下
    template<size_t N>
    using StringLiteralHelper = const char[N];

    struct StringLiteral 
    {
        const char* data;
        size_t size;
        template<size_t N>
        StringLiteral(StringLiteralHelper<N> const& s) noexcept : data(s), size(N - 1) {}
    };

    template<typename T>
    using is_supported_parameter_t = typename std::enable_if<
            std::is_same<float, T>::value ||
            std::is_same<int32_t, T>::value ||
            std::is_same<uint32_t, T>::value ||
            std::is_same<math::int2, T>::value ||
            std::is_same<math::int3, T>::value ||
            std::is_same<math::int4, T>::value ||
            std::is_same<math::uint2, T>::value ||
            std::is_same<math::uint3, T>::value ||
            std::is_same<math::uint4, T>::value ||
            std::is_same<math::float2, T>::value ||
            std::is_same<math::float3, T>::value ||
            std::is_same<math::float4, T>::value ||
            std::is_same<math::mat4f, T>::value ||
            std::is_same<bool, T>::value ||
            std::is_same<math::bool2, T>::value ||
            std::is_same<math::bool3, T>::value ||
            std::is_same<math::bool4, T>::value ||
            std::is_same<math::mat3f, T>::value
    >::type;

    // 这里的使用原理是,当T不是支持的类型,上面的 std::enable_if<>::type 就不存在,编译会报错.
    // 下面的模版函数setParameter的第二个 typename之所以没有命名,是因为不需要用到这个 typename,只是给编译器用的.
    template<typename T, typename = is_supported_parameter_t<T>>
    void setParameter(const char* name, size_t nameLength, T const& value);
}

//----------------------------------------------------------------------------------------------------------


class UTILS_PUBLIC Viewport : public backend::Viewport 
{
    // 以下操作符应该完全可以定义为 class operator function.效果是一样的.
    friend bool operator==(Viewport const& lhs, Viewport const& rhs) noexcept {
        return (&rhs == &lhs) ||
               (rhs.left == lhs.left && rhs.bottom == lhs.bottom &&
                rhs.width == lhs.width && rhs.height == lhs.height);
    }

    friend bool operator!=(Viewport const& lhs, Viewport const& rhs) noexcept {
        return !(rhs == lhs);
    }
}
