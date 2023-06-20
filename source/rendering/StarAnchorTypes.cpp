#include "StarAnchorTypes.hpp"

namespace Star {

EnumMap<HorizontalAnchor> const HorizontalAnchorNames{
    {HorizontalAnchor::LeftAnchor, "left"},
    {HorizontalAnchor::HMidAnchor, "mid"},
    {HorizontalAnchor::RightAnchor, "right"},
};

EnumMap<VerticalAnchor> const VerticalAnchorNames{
    {VerticalAnchor::BottomAnchor, "bottom"}, {VerticalAnchor::VMidAnchor, "mid"}, {VerticalAnchor::TopAnchor, "top"},
};
}
