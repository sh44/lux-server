#pragma once

struct ActionType
{
    void (&action)();
    String name;
};
