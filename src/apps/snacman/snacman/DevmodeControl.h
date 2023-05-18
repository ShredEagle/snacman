namespace ad {
namespace snac {


// Could be made dynamic later
constexpr bool isDevmode()
{
#if defined(SNACMAN_DEVMODE)
    return true;
#else
    return false;
#endif
}


} // namespace snac
} // namespace ad
