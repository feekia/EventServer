#include<gtest/gtest.h>
int add(int a,int b){
    return a+b;
}

int minus(int a,int b){
    return a-b;
}
TEST(testCase,test0){
    EXPECT_EQ(add(2,3),5);
}

TEST(testCase,tes1){
    EXPECT_EQ(minus(2,3),2);
}
int main(int argc,char **argv){
  testing::InitGoogleTest(&argc,argv);
  return RUN_ALL_TESTS();
}