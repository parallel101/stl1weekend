#pragma once

template <class ...Ts>
union Variant {
    Ts ...m_members;
};
