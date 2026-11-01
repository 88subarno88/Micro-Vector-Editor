#include <gtest/gtest.h>

#include "db/Models.h"

using namespace vec;

TEST(ModelsTest, RecognisesUuids) {
    EXPECT_TRUE(isUuid("550e8400-e29b-41d4-a716-446655440000"));
    EXPECT_TRUE(isUuid("550E8400-E29B-41D4-A716-446655440000"));
    for (const char* text : {"", "not-a-uuid", "550e8400e29b41d4a716446655440000",
                             "550e8400-e29b-41d4-a716-44665544000", "550e8400-e29b-41d4-a716-4466554400000",
                             "550e8400-e29b-41d4-a716-44665544000g", "'; DROP TABLE users; --"}) {
        EXPECT_FALSE(isUuid(text)) << text;
    }
}

TEST(ModelsTest, EveryRoleConvertsBothWays) {
    for (const Role role : {Role::Viewer, Role::Editor, Role::Owner}) {
        EXPECT_EQ(roleFromString(toString(role)), role);
    }
}

TEST(ModelsTest, RoleNamesMatchTheDatabaseConstraint) {
    EXPECT_EQ(toString(Role::Viewer), "viewer");
    EXPECT_EQ(toString(Role::Editor), "editor");
    EXPECT_EQ(toString(Role::Owner), "owner");
}

TEST(ModelsTest, UnknownRolesAreRejected) {
    for (const char* text : {"", "admin", "Owner", "owner "}) {
        EXPECT_EQ(roleFromString(text), std::nullopt) << text;
    }
}

TEST(ModelsTest, RolesOrderByAccess) {
    EXPECT_LT(Role::Viewer, Role::Editor);
    EXPECT_LT(Role::Editor, Role::Owner);
}
