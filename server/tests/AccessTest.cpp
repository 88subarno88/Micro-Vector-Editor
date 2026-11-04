#include <gtest/gtest.h>

#include "auth/Access.h"

using namespace vec;

TEST(AccessTest, NonMembersCannotTellTheDocumentExists) {
    for (const Role minimum : {Role::Viewer, Role::Editor, Role::Owner}) {
        EXPECT_EQ(decideAccess(std::nullopt, minimum), Access::NotFound);
    }
}

TEST(AccessTest, EachRoleHasExactlyTheAccessAtOrBelowIt) {
    EXPECT_EQ(decideAccess(Role::Viewer, Role::Viewer), Access::Allowed);
    EXPECT_EQ(decideAccess(Role::Viewer, Role::Editor), Access::Forbidden);
    EXPECT_EQ(decideAccess(Role::Viewer, Role::Owner), Access::Forbidden);

    EXPECT_EQ(decideAccess(Role::Editor, Role::Viewer), Access::Allowed);
    EXPECT_EQ(decideAccess(Role::Editor, Role::Editor), Access::Allowed);
    EXPECT_EQ(decideAccess(Role::Editor, Role::Owner), Access::Forbidden);

    EXPECT_EQ(decideAccess(Role::Owner, Role::Viewer), Access::Allowed);
    EXPECT_EQ(decideAccess(Role::Owner, Role::Editor), Access::Allowed);
    EXPECT_EQ(decideAccess(Role::Owner, Role::Owner), Access::Allowed);
}
