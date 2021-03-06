#include "software/handheld_controller/controller_primitive_generator.h"

#include "software/proto/message_translation/tbots_protobuf.h"
#include "software/proto/primitive/primitive_msg_factory.h"

ControllerPrimitiveGenerator::ControllerPrimitiveGenerator(
    std::shared_ptr<const HandheldControllerConfig> controller_input_config)
    : controller_input_config(controller_input_config)
{
}

void ControllerPrimitiveGenerator::onValueReceived(ControllerInput controller_input)
{
    unsigned int robot_id  = controller_input_config->RobotId()->value();
    auto primitive_set_msg = std::make_unique<TbotsProto::PrimitiveSet>();
    *(primitive_set_msg->mutable_time_sent()) = *createCurrentTimestamp();

    auto &robot_primitives_map = *primitive_set_msg->mutable_robot_primitives();
    robot_primitives_map[robot_id] =
        *createPrimitiveFromControllerInput(controller_input, controller_input_config);
    Subject<TbotsProto::PrimitiveSet>::sendValueToObservers(*primitive_set_msg);
}

std::unique_ptr<TbotsProto::Primitive>
ControllerPrimitiveGenerator::createPrimitiveFromControllerInput(
    const ControllerInput &controller_input,
    std::shared_ptr<const HandheldControllerConfig> controller_input_config)
{
    if (controller_input.isKickButtonPressed())
    {
        double kick_speed = controller_input_config->KickSpeedMetersPerSecond()->value();
        auto kick_primitive = createKickPrimitive(Point(0, 0), Angle::zero(), kick_speed);
        return kick_primitive;
    }
    else if (controller_input.isChipButtonPressed())
    {
        double chip_distance = controller_input_config->ChipDistanceMeters()->value();
        auto chip_primitive =
            createChipPrimitive(Point(0, 0), Angle::zero(), chip_distance);
        return chip_primitive;
    }
    else
    {
        unsigned int dribbler_rpm = controller_input.isDribblerButtonPressed()
                                        ? controller_input_config->DribblerRpm()->value()
                                        : 0;
        double x_velocity = controller_input.getLinearMotionX() *
                            controller_input_config->MaxLinearSpeed()->value();
        double y_velocity = controller_input.getLinearMotionY() *
                            controller_input_config->MaxLinearSpeed()->value();
        double angular_velocity = controller_input.getAngularMotion() *
                                  controller_input_config->MaxAngularSpeed()->value();
        auto direct_velocity_primitive = createDirectControlPrimitive(
            Vector(x_velocity, y_velocity),
            AngularVelocity::fromRadians(angular_velocity), dribbler_rpm);
        return direct_velocity_primitive;
    }
}

std::unique_ptr<TbotsProto::Primitive>
ControllerPrimitiveGenerator::createDirectControlPrimitive(
    const Vector &velocity, AngularVelocity angular_velocity, double dribbler_speed_rpm)
{
    auto direct_control_primitive_msg = std::make_unique<TbotsProto::Primitive>();

    auto direct_velocity_control =
        std::make_unique<TbotsProto::DirectControlPrimitive::DirectVelocityControl>();
    *(direct_velocity_control->mutable_velocity()) = *createVectorProto(velocity);
    *(direct_velocity_control->mutable_angular_velocity()) =
        *createAngularVelocityProto(angular_velocity);
    *(direct_control_primitive_msg->mutable_direct_control()
          ->mutable_direct_velocity_control()) = *direct_velocity_control;

    direct_control_primitive_msg->mutable_direct_control()->set_dribbler_speed_rpm(
        static_cast<float>(dribbler_speed_rpm));
    return direct_control_primitive_msg;
}
