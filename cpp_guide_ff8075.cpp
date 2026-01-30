// Learning Objective: Build a compile-time type-safe unit system using C++ templates and std::ratio
// to prevent incorrect measurement conversions (e.g., adding meters to seconds) at compile time.
// This teaches how to leverage C++'s powerful type system to enforce physical consistency in your code.

#include <iostream>      // For printing output
#include <ratio>         // For compile-time rational numbers (exponents of base units)
#include <string>        // For displaying unit strings in print_quantity helper

// 1. Define Base Unit Exponents
//    We represent the "dimensions" of our units using std::ratio.
//    std::ratio<Num, Den> denotes the exponent of a base unit (e.g., length^1, time^-1).
//    Exponent<N> is a shorthand for std::ratio<N, 1>, making it clearer for integer exponents.
template <int N>
using Exponent = std::ratio<N, 1>;

// 2. Define the Unit Type (Compile-time type tag)
//    This struct defines a unique type for each combination of base unit exponents.
//    It carries the dimensional information (length, mass, time, current, etc.) at compile time.
//    By default, all exponents are 0, representing a dimensionless quantity.
template <typename LengthExp = Exponent<0>,  // Exponent for length (e.g., meters)
          typename MassExp = Exponent<0>,    // Exponent for mass (e.g., kilograms)
          typename TimeExp = Exponent<0>,    // Exponent for time (e.g., seconds)
          typename CurrentExp = Exponent<0>  // Exponent for electric current (e.g., amperes)
          >
struct Unit {
    // No data members; this struct's sole purpose is to define a type.
    // These aliases provide access to the specific exponent ratios for this unit.
    using LengthRatio = LengthExp;
    using MassRatio = MassExp;
    using TimeRatio = TimeExp;
    using CurrentRatio = CurrentExp;
};

// 3. Define specific common unit types for convenience
//    These are aliases to make `Quantity` declarations more readable.
using Meter = Unit<Exponent<1>>;                                            // Length^1
using Kilogram = Unit<Exponent<0>, Exponent<1>>;                           // Mass^1
using Second = Unit<Exponent<0>, Exponent<0>, Exponent<1>>;                // Time^1
using Ampere = Unit<Exponent<0>, Exponent<0>, Exponent<0>, Exponent<1>>;   // Current^1

// Derived units examples
using Velocity = Unit<Exponent<1>, Exponent<0>, Exponent<-1>>;          // m/s = Length^1 * Time^-1
using Acceleration = Unit<Exponent<1>, Exponent<0>, Exponent<-2>>;      // m/s^2 = Length^1 * Time^-2
using Area = Unit<Exponent<2>>;                                         // m^2 = Length^2
using Force = Unit<Exponent<1>, Exponent<1>, Exponent<-2>>;            // N = Length^1 * Mass^1 * Time^-2 (kg*m/s^2)
using Dimensionless = Unit<>;                                           // Unitless quantity (all exponents are 0)

// 4. Define the Quantity Type
//    This struct holds the actual numerical value and is templated
//    on the `UnitType` to associate the value with its physical dimensions.
template <typename UnitType, typename ValueType = double>
struct Quantity {
    ValueType value; // The actual measurement value (e.g., 10.5)

    // Constructor to initialize a Quantity
    explicit Quantity(ValueType val) : value(val) {}

    // Arithmetic Operator Overloads: The core of compile-time type safety.

    // Addition operator (+): Requires quantities to have the EXACT SAME unit type.
    // If UnitType and OtherUnitType differ, the compiler will error here, preventing
    // operations like 'meters + seconds'. This enforces dimensional consistency.
    Quantity<UnitType, ValueType> operator+(const Quantity<UnitType, ValueType>& other) const {
        return Quantity<UnitType, ValueType>(value + other.value);
    }

    // Subtraction operator (-): Similar to addition, enforces same unit type.
    Quantity<UnitType, ValueType> operator-(const Quantity<UnitType, ValueType>& other) const {
        return Quantity<UnitType, ValueType>(value - other.value);
    }

    // Multiplication operator (*): Combines units by ADDING their base unit exponents.
    // E.g., Meter * Meter results in an Area (Length^1 * Length^1 = Length^2).
    // A new `Unit` type is created for the result based on summed exponents using `std::ratio_add`.
    template <typename OtherUnitType>
    auto operator*(const Quantity<OtherUnitType, ValueType>& other) const {
        // std::ratio_add calculates (N1/D1) + (N2/D2) and returns a new std::ratio type.
        using ResultUnit = Unit<
            typename std::ratio_add<typename UnitType::LengthRatio, typename OtherUnitType::LengthRatio>::type,
            typename std::ratio_add<typename UnitType::MassRatio, typename OtherUnitType::MassRatio>::type,
            typename std::ratio_add<typename UnitType::TimeRatio, typename OtherUnitType::TimeRatio>::type,
            typename std::ratio_add<typename UnitType::CurrentRatio, typename OtherUnitType::CurrentRatio>::type
        >;
        return Quantity<ResultUnit, ValueType>(value * other.value);
    }

    // Division operator (/): Combines units by SUBTRACTING their base unit exponents.
    // E.g., Meter / Second results in Velocity (Length^1 / Time^1 = Length^1 * Time^-1).
    // A new `Unit` type is created for the result based on subtracted exponents using `std::ratio_subtract`.
    template <typename OtherUnitType>
    auto operator/(const Quantity<OtherUnitType, ValueType>& other) const {
        // std::ratio_subtract calculates (N1/D1) - (N2/D2) and returns a new std::ratio type.
        using ResultUnit = Unit<
            typename std::ratio_subtract<typename UnitType::LengthRatio, typename OtherUnitType::LengthRatio>::type,
            typename std::ratio_subtract<typename UnitType::MassRatio, typename OtherUnitType::MassRatio>::type,
            typename std::ratio_subtract<typename UnitType::TimeRatio, typename OtherUnitType::TimeRatio>::type,
            typename std::ratio_subtract<typename UnitType::CurrentRatio, typename OtherUnitType::CurrentRatio>::type
        >;
        return Quantity<ResultUnit, ValueType>(value / other.value);
    }
};

// 5. Helper function for displaying quantities (for demonstration purposes)
//    This helper takes a string for the unit symbol, as the UnitType itself is a compile-time construct.
template <typename UnitType, typename ValueType>
void print_quantity(const Quantity<UnitType, ValueType>& q, const std::string& unit_str) {
    std::cout << q.value << " " << unit_str << std::endl;
}

// 6. Main function for example usage
int main() {
    std::cout << "--- C++ Compile-Time Type-Safe Unit System Tutorial ---" << std::endl;

    // Create various quantities with their specific unit types
    Quantity<Meter> length1(10.0);
    Quantity<Meter> length2(5.0);
    Quantity<Second> time1(2.0);
    Quantity<Kilogram> mass1(70.0);

    print_quantity(length1, "meters");
    print_quantity(time1, "seconds");
    print_quantity(mass1, "kilograms");
    std::cout << std::endl;

    // Example: Type-safe addition/subtraction (only works with identical units)
    std::cout << "Adding lengths: ";
    auto total_length = length1 + length2; // Both are Quantity<Meter>, so addition is allowed.
    print_quantity(total_length, "meters"); // Result is also a Quantity<Meter>

    std::cout << "Subtracting lengths: ";
    auto remaining_length = length1 - length2; // Also requires matching units.
    print_quantity(remaining_length, "meters");
    std::cout << std::endl;

    // Example: Compile-time error for incorrect addition/subtraction
    // UNCOMMENT THE LINE BELOW TO SEE A COMPILE-TIME ERROR:
    // auto invalid_sum = length1 + time1; // ERROR: Cannot add meters and seconds!
    // This is the core safety feature: physically inconsistent operations are prevented by the compiler.

    // Example: Multiplication and division (deriving new units)
    std::cout << "Multiplying length by length (Area): ";
    auto room_area = length1 * length2; // Resulting type is Quantity<Area> (Length^2)
    print_quantity(room_area, "m^2 (Area)");

    std::cout << "Dividing length by time (Velocity): ";
    auto car_speed = length1 / time1; // Resulting type is Quantity<Velocity> (Length^1 * Time^-1)
    print_quantity(car_speed, "m/s (Velocity)");

    // Example: More complex derived unit (Force)
    std::cout << "Calculating Force (Mass * Acceleration):" << std::endl;
    Quantity<Acceleration> gravity(9.81); // Define an acceleration quantity
    print_quantity(gravity, "m/s^2 (Acceleration)");

    auto weight = mass1 * gravity; // Resulting type is Quantity<Force> (Mass^1 * Length^1 * Time^-2)
    print_quantity(weight, "N (Force)"); // N = kg * m / s^2

    // Example: Dimensionless quantity
    std::cout << "Dimensionless quantity: ";
    Quantity<Dimensionless> ratio(0.5); // All exponents are 0 for Dimensionless
    print_quantity(ratio, "unitless");

    std::cout << "\nEnd of tutorial. Experiment by uncommenting the error line!" << std::endl;
    return 0;
}